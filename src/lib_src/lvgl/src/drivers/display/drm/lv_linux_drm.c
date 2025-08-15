/**
 * @file lv_linux_drm.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_linux_drm.h"
#if LV_USE_LINUX_DRM

#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <poll.h>
#include <malloc.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include <drm_mode.h>

#include "../../../stdlib/lv_sprintf.h"
#include "../../../draw/lv_draw_buf.h"

#if LV_LINUX_DRM_GBM_BUFFERS

#include <gbm.h>
#include <linux/dma-buf.h>
#include <sys/ioctl.h>

#endif

/*********************
 *      DEFINES
 *********************/
#define NUM_DUMB_BO 3
#define ALIGN(x, a) (((x) + (a - 1)) & ~(a - 1))
#if (LV_COLOR_DEPTH == 16)
#define DRM_FORMAT DRM_FORMAT_RGB565
#define RGA_FORMAT RK_FORMAT_BGR_565
#elif (LV_COLOR_DEPTH == 24)
#define DRM_FORMAT DRM_FORMAT_RGB888
#define RGA_FORMAT RK_FORMAT_BGR_888
#elif (LV_COLOR_DEPTH == 32)
#define DRM_FORMAT DRM_FORMAT_ARGB8888
#define RGA_FORMAT RK_FORMAT_BGRA_8888
#else
#error LV_COLOR_DEPTH not supported
#endif

#define BUFFER_CNT 2

/**********************
 *      TYPEDEFS
 **********************/
typedef struct drm_bo
{
    int fd;
    void *ptr;
    size_t size;
    size_t offset;
    size_t pitch;
    unsigned int handle;
    int fb_id;
    int buf_fd;
    int w;
    int h;
} drm_bo_t;

typedef struct drm_device
{
    int fd;
    struct
    {
        int width;
        int height;

        int hdisplay;
        int vdisplay;
    } mode;

    drmModeResPtr res;

    int connector_id;
    int encoder_id;
    int crtc_id;
    int plane_id;
    int last_fb_id;
    uint32_t *con_ids;
    unsigned int num_cons;
    drmModeModeInfo mode_info;

    int waiting_for_flip;
    struct pollfd drm_pollfd;
    drmEventContext drm_evctx;

    int lcd_sw;
    char *drm_buff;
    lv_color_t *disp_buf;
    int disp_rot;

    int quit;
    pthread_t pid;
    pthread_mutex_t mutex;
    int draw_update;

    drm_bo_t *vop_buf[2];
#if LV_DRM_USE_RGA
    drm_bo_t *gbo;
#endif
} drm_device_t;
/* Called by LVGL when there is something that needs redrawing
 * it sets the active buffer. if GBM buffers are used, it issues a DMA_BUF_SYNC
 * ioctl call to lock the buffer for CPU access, the buffer is unlocked just
 * before the atomic commit */
// static void drm_dmabuf_set_active_buf(lv_event_t * event)
// {

//     drm_dev_t * drm_dev;
//     lv_display_t * disp;
//     lv_draw_buf_t * act_buf;
//     int i;

//     disp = (lv_display_t *) lv_event_get_current_target(event);
//     drm_dev = (drm_dev_t *) lv_display_get_driver_data(disp);
//     act_buf = lv_display_get_buf_active(disp);

//     if(drm_dev->act_buf == NULL) {

//         for(i = 0; i < BUFFER_CNT; i++) {
//             if(act_buf->unaligned_data == drm_dev->drm_bufs[i].map) {
//                 drm_dev->act_buf = &drm_dev->drm_bufs[i];
//                 LV_LOG_TRACE("Set active buffer idx: %d", i);
//                 break;
//             }
//         }

// #if LV_LINUX_DRM_GBM_BUFFERS

//         struct dma_buf_sync sync_req;
//         sync_req.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW;
//         int res;

//         if((res = ioctl(drm_dev->act_buf->handle, DMA_BUF_IOCTL_SYNC, &sync_req)) != 0) {
//             LV_LOG_ERROR("Failed to start DMA-BUF R/W SYNC res: %d", res);
//         }
// #endif

//     }
//     else {

//         LV_LOG_TRACE("active buffer already set");
//     }

// }

static int bo_map(drm_device_t *dev, drm_bo_t *bo)
{
    struct drm_mode_map_dumb arg = {
        .handle = bo->handle,
    };
    struct drm_prime_handle fd_args = {
        .fd = -1,
        .handle = bo->handle,
        .flags = 0,
    };
    int ret;

    ret = drmIoctl(dev->fd, DRM_IOCTL_MODE_MAP_DUMB, &arg);
    if (ret)
        return ret;

    ret = drmIoctl(dev->fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &fd_args);
    if (ret)
    {
        LV_LOG_ERROR("handle_to_fd failed ret=%d, handle=%x", ret, fd_args.handle);
        return -1;
    }
    bo->buf_fd = fd_args.fd;

    bo->ptr = mmap(0, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED,
                   dev->fd, arg.offset);

    if (bo->ptr == MAP_FAILED)
    {
        bo->ptr = NULL;
        return -1;
    }

    return 0;
}

static void bo_unmap(drm_device_t *dev, drm_bo_t *bo)
{
    if (dev == NULL)
        return;
    if (!bo->ptr)
        return;

    drmUnmap(bo->ptr, bo->size);
    if (bo->buf_fd > 0)
        close(bo->buf_fd);
    bo->ptr = NULL;
}

void bo_destroy(drm_device_t *dev, drm_bo_t *bo)
{
    struct drm_mode_destroy_dumb arg = {
        .handle = bo->handle,
    };

    if (bo->fb_id)
        drmModeRmFB(dev->fd, bo->fb_id);

    bo_unmap(dev, bo);

    if (bo->handle)
        drmIoctl(dev->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &arg);

    free(bo);
}

static drm_bo_t *bo_create(drm_device_t *dev, int width, int height, int format)
{
    struct drm_mode_create_dumb arg = {
        .bpp = LV_COLOR_DEPTH,
        .width = ALIGN(width, 16),
        .height = ALIGN(height, 16),
    };
    drm_bo_t *bo;
    uint32_t handles[4] = {0}, pitches[4] = {0}, offsets[4] = {0};
    int ret;

    bo = malloc(sizeof(drm_bo_t));
    if (bo == NULL)
    {
        LV_LOG_ERROR("allocate bo failed");
        return NULL;
    }
    memset(bo, 0, sizeof(*bo));
    if (format == DRM_FORMAT_NV12)
    {
        arg.bpp = 8;
        arg.height = height * 3 / 2;
    }

    ret = drmIoctl(dev->fd, DRM_IOCTL_MODE_CREATE_DUMB, &arg);
    if (ret)
    {
        LV_LOG_ERROR("create dumb failed");
        goto err;
    }

    bo->fd = dev->fd;
    bo->handle = arg.handle;
    bo->size = arg.size;
    bo->pitch = arg.pitch;
    bo->w = width;
    bo->h = height;

    ret = bo_map(dev, bo);
    if (ret)
    {
        LV_LOG_ERROR("map bo failed");
        goto err;
    }

    switch (format)
    {
    case DRM_FORMAT_NV12:
    case DRM_FORMAT_NV16:
        handles[0] = bo->handle;
        pitches[0] = bo->pitch;
        offsets[0] = 0;
        handles[1] = bo->handle;
        pitches[1] = pitches[0];
        offsets[1] = pitches[0] * height;
        break;
    case DRM_FORMAT_RGB332:
        handles[0] = bo->handle;
        pitches[0] = bo->pitch;
        offsets[0] = 0;
        break;
    case DRM_FORMAT_RGB565:
    case DRM_FORMAT_BGR565:
        handles[0] = bo->handle;
        pitches[0] = bo->pitch;
        offsets[0] = 0;
        break;
    case DRM_FORMAT_RGB888:
    case DRM_FORMAT_BGR888:
        handles[0] = bo->handle;
        pitches[0] = bo->pitch;
        offsets[0] = 0;
        break;
    case DRM_FORMAT_ARGB8888:
    case DRM_FORMAT_ABGR8888:
    case DRM_FORMAT_RGBA8888:
    case DRM_FORMAT_BGRA8888:
        handles[0] = bo->handle;
        pitches[0] = bo->pitch;
        offsets[0] = 0;
        break;
    }
    ret = drmModeAddFB2(dev->fd, width, height, format, handles,
                        pitches, offsets, (uint32_t *)&bo->fb_id, 0);
    if (ret)
    {
        LV_LOG_ERROR("add fb failed");
        goto err;
    }
    LV_LOG_USER("Created bo: %d, %dx%d", bo->fb_id, width, height);
    return bo;
err:
    bo_destroy(dev, bo);
    return NULL;
}

drm_bo_t *malloc_drm_bo(drm_device_t *dev, int width, int height, int format)
{
    return bo_create(dev, width, height, format);
}

void free_drm_bo(drm_device_t *dev, drm_bo_t *bo)
{
    if (bo)
        bo_destroy(dev, bo);
}

static int drm_get_preferred_connector(void)
{
    const char *path;
    char buf[256] = "\0";
    int fd;

#define DRM_CONNECTOR_CFG_PATH_ENV "DRM_CONNECTOR_CFG_PATH"
#define DRM_CONNECTOR_CFG_PATH_DEFAULT "/tmp/drm_connector.cfg"
    path = getenv(DRM_CONNECTOR_CFG_PATH_ENV);
    if (!path)
        path = DRM_CONNECTOR_CFG_PATH_DEFAULT;
    fd = open(path, O_RDONLY);
    if (read(fd, buf, sizeof(buf)) != sizeof(buf))
    {
        LV_LOG_TRACE("read failed, use default connector");
    }
    close(fd);

    if (!buf[0])
        return -1;

    return atoi(buf);
}

static int drm_get_preferred_mode(int *width, int *height)
{
    const char *path;
    char buf[256] = "\0";
    int fd, w, h;

#define DRM_MODE_CFG_PATH_ENV "DRM_CONNECTOR_CFG_PATH"
#define DRM_MODE_CFG_PATH_DEFAULT "/tmp/drm_mode.cfg"
    path = getenv(DRM_MODE_CFG_PATH_ENV);
    if (!path)
        path = DRM_MODE_CFG_PATH_DEFAULT;

    fd = open(path, O_RDONLY);
    if (read(fd, buf, sizeof(buf)) != sizeof(buf))
    {
        LV_LOG_TRACE("read failed, use default mode");
    }
    close(fd);
    if (!buf[0])
        return -1;
    if (2 != sscanf(buf, "%dx%d", &w, &h))
        return -1;
    *width = w;
    *height = h;

    return 0;
}

static drmModeConnectorPtr drm_get_connector(drm_device_t *dev, int connector_id)
{
    drmModeConnectorPtr conn;

    conn = drmModeGetConnector(dev->fd, connector_id);
    if (!conn)
        return NULL;

    LV_LOG_TRACE("Connector id: %d, %sconnected, modes: %d", connector_id,
                 (conn->connection == DRM_MODE_CONNECTED) ? "" : "dis",
                 conn->count_modes);
    if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes)
        return conn;

    drmModeFreeConnector(conn);
    return NULL;
}

static drmModeConnectorPtr drm_find_best_connector(drm_device_t *dev)
{
    drmModeResPtr res = dev->res;
    drmModeConnectorPtr conn;
    int i, preferred_connector_id = drm_get_preferred_connector();

    LV_LOG_TRACE("Preferred connector id: %d", preferred_connector_id);
    conn = drm_get_connector(dev, preferred_connector_id);
    if (conn)
        return conn;

    dev->num_cons = 1;
    dev->con_ids = calloc(1, sizeof(*dev->con_ids));
    for (i = 0; i < res->count_connectors; i++)
    {
        conn = drm_get_connector(dev, res->connectors[i]);
        if (conn)
        {
            dev->con_ids[0] = conn->connector_id;
            memcpy(&dev->mode_info, &conn->modes[0], sizeof(dev->mode_info));
            return conn;
        }
    }
    return NULL;
}

static drmModeCrtcPtr drm_find_best_crtc(drm_device_t *dev, drmModeConnectorPtr conn)
{
    drmModeResPtr res = dev->res;
    drmModeEncoderPtr encoder;
    drmModeCrtcPtr crtc;
    int i, preferred_crtc_id = 0;
    int crtcs_for_connector = 0;

    encoder = drmModeGetEncoder(dev->fd, conn->encoder_id);
    if (encoder)
    {
        preferred_crtc_id = encoder->crtc_id;
        drmModeFreeEncoder(encoder);
        LV_LOG_TRACE("Preferred crtc: %d", preferred_crtc_id);
    }
    crtc = drmModeGetCrtc(dev->fd, preferred_crtc_id);
    if (crtc)
        return crtc;

    for (i = 0; i < res->count_encoders; i++)
    {
        encoder = drmModeGetEncoder(dev->fd, res->encoders[i]);
        if (encoder)
            crtcs_for_connector |= encoder->possible_crtcs;
        drmModeFreeEncoder(encoder);
    }
    LV_LOG_TRACE("Possible crtcs: %x", crtcs_for_connector);
    if (!crtcs_for_connector)
        return NULL;

    return drmModeGetCrtc(dev->fd, res->crtcs[ffs(crtcs_for_connector) - 1]);
}

int drm_plane_is_primary(drm_device_t *dev, int plane_id)
{
    drmModeObjectPropertiesPtr props;
    drmModePropertyPtr prop;
    unsigned int i;
    int type = 0;
    props = drmModeObjectGetProperties(dev->fd, plane_id,
                                       DRM_MODE_OBJECT_PLANE);
    if (!props)
        return 0;
    for (i = 0; i < props->count_props; i++)
    {
        prop = drmModeGetProperty(dev->fd, props->props[i]);
        if (prop && !strcmp(prop->name, "type"))
            type = props->prop_values[i];
        drmModeFreeProperty(prop);
    }
    LV_LOG_TRACE("Plane: %d, type: %d", plane_id, type);

    drmModeFreeObjectProperties(props);
    return type == DRM_PLANE_TYPE_PRIMARY;
}

int drm_plane_is_overlay(drm_device_t *dev, int plane_id)
{
    drmModeObjectPropertiesPtr props;
    drmModePropertyPtr prop;
    unsigned int i;
    int type = 0;

    props = drmModeObjectGetProperties(dev->fd, plane_id,
                                       DRM_MODE_OBJECT_PLANE);
    if (!props)
        return 0;

    for (i = 0; i < props->count_props; i++)
    {
        prop = drmModeGetProperty(dev->fd, props->props[i]);
        if (prop && !strcmp(prop->name, "type"))
            type = props->prop_values[i];
        drmModeFreeProperty(prop);
    }
    LV_LOG_TRACE("Plane: %d, type: %d", plane_id, type);
    drmModeFreeObjectProperties(props);
    return type == DRM_PLANE_TYPE_OVERLAY;
}

int drm_plane_is_cursor(drm_device_t *dev, int plane_id)
{
    drmModeObjectPropertiesPtr props;
    drmModePropertyPtr prop;
    unsigned int i;
    int type = 0;

    props = drmModeObjectGetProperties(dev->fd, plane_id,
                                       DRM_MODE_OBJECT_PLANE);
    if (!props)
        return 0;

    for (i = 0; i < props->count_props; i++)
    {
        prop = drmModeGetProperty(dev->fd, props->props[i]);
        if (prop && !strcmp(prop->name, "type"))
            type = props->prop_values[i];
        drmModeFreeProperty(prop);
    }
    LV_LOG_TRACE("Plane: %d, type: %d", plane_id, type);
    drmModeFreeObjectProperties(props);
    return type == DRM_PLANE_TYPE_CURSOR;
}

static drmModePlanePtr drm_get_plane(drm_device_t *dev, int plane_id, int pipe)
{
    drmModePlanePtr plane;
    char *set_plane;
    plane = drmModeGetPlane(dev->fd, plane_id);
    if (!plane)
        return NULL;

    LV_LOG_TRACE("Check plane: %d, possible_crtcs: %x", plane_id,
                 plane->possible_crtcs);
    set_plane = getenv("LV_DRIVERS_SET_PLANE");
    if (set_plane == NULL)
    {
        LV_LOG_USER("LV_DRIVERS_SET_PLANE not be set, use DRM_PLANE_TYPE_PRIMARY");
        if (drm_plane_is_primary(dev, plane_id))
        {
            if (plane->possible_crtcs & (1 << pipe))
                return plane;
        }
        goto end;
    }
    if (!strcmp("OVERLAY", set_plane))
    {
        LV_LOG_USER("LV_DRIVERS_SET_PLANE = DRM_PLANE_TYPE_OVERLAY");
        if (drm_plane_is_overlay(dev, plane_id))
        {
            if (plane->possible_crtcs & (1 << pipe))
                return plane;
        }
    }
    else if (!strcmp("PRIMARY", set_plane))
    {
        LV_LOG_USER("LV_DRIVERS_SET_PLANE = DRM_PLANE_TYPE_PRIMARY");
        if (drm_plane_is_primary(dev, plane_id))
        {
            if (plane->possible_crtcs & (1 << pipe))
                return plane;
        }
    }
    else
    {
        LV_LOG_USER("LV_DRIVERS_SET_PLANE set err, use DRM_PLANE_TYPE_PRIMARY");
        if (drm_plane_is_primary(dev, plane_id))
        {
            if (plane->possible_crtcs & (1 << pipe))
                return plane;
        }
    }

end:
    drmModeFreePlane(plane);
    return NULL;
}

static drmModePlanePtr drm_find_best_plane(drm_device_t *dev, drmModeCrtcPtr crtc)
{
    drmModeResPtr res = dev->res;
    drmModePlaneResPtr pres;
    drmModePlanePtr plane;
    unsigned int i;
    int pipe;
    for (pipe = 0; pipe < res->count_crtcs; pipe++)
    {
        if (crtc->crtc_id == res->crtcs[pipe])
            break;
    }
    if (pipe == res->count_crtcs)
        return NULL;
    pres = drmModeGetPlaneResources(dev->fd);
    if (!pres)
        return NULL;

    for (i = 0; i < pres->count_planes; i++)
    {
        plane = drm_get_plane(dev, pres->planes[i], pipe);
        if (plane)
        {
            drmModeFreePlaneResources(pres);
            return plane;
        }
        drmModeFreePlane(plane);
    }
    drmModeFreePlaneResources(pres);
    return NULL;
}
static drmModeModeInfoPtr drm_find_best_mode(drm_device_t *dev, drmModeConnectorPtr conn)
{
    drmModeModeInfoPtr mode;
    int i, preferred_width = 1920, preferred_height = 1080;

    if (dev == NULL)
        return 0;
    drm_get_preferred_mode(&preferred_width, &preferred_height);
    LV_LOG_TRACE("Preferred mode: %dx%d", preferred_width, preferred_height);

    mode = &conn->modes[0];
    for (i = 0; i < conn->count_modes; i++)
    {
        LV_LOG_TRACE("Check mode: %dx%d",
                     conn->modes[i].hdisplay, conn->modes[i].vdisplay);
        if (conn->modes[i].hdisplay == preferred_width &&
            conn->modes[i].vdisplay == preferred_height)
        {
            mode = &conn->modes[i];
            break;
        }
    }
    return mode;
}

static int drm_get_preferred_fb_mode(int *width, int *height)
{
    char *buf;
    int w, h;
    buf = getenv("LVGL_DRM_FB_MODE");
    if (!buf)
        return -1;
    if (2 != sscanf(buf, "%dx%d", &w, &h))
        return -1;
    LV_LOG_TRACE("Preferred fb mode: %dx%d", w, h);
    *width = w;
    *height = h;
    return 0;
}

static void drm_setup_fb_mode(drm_device_t *dev)
{
    drmModeResPtr res = dev->res;
    drmModeConnectorPtr conn;
    drmModeModeInfoPtr mode;
    int i;
    if (dev->mode.width && dev->mode.height)
        return;
    if (!drm_get_preferred_fb_mode(&dev->mode.width, &dev->mode.height))
        return;
    dev->mode.width = dev->mode.hdisplay;
    dev->mode.height = dev->mode.vdisplay;

    for (i = 0; i < res->count_connectors; i++)
    {
        conn = drm_get_connector(dev, res->connectors[i]);
        if (!conn)
            continue;
        mode = drm_find_best_mode(dev, conn);
        if (mode)
        {
            LV_LOG_TRACE("Best mode for connector(%d): %dx%d",
                         conn->connector_id, mode->hdisplay, mode->vdisplay);
            if (dev->mode.width > mode->hdisplay ||
                dev->mode.height > mode->vdisplay)
            {
                dev->mode.width = mode->hdisplay;
                dev->mode.height = mode->vdisplay;
            }
        }
        drmModeFreeConnector(conn);
    }
}

static void configure_plane_zpos(drm_device_t *self, int plane_id, uint64_t zpos)
{
    drmModeObjectPropertiesPtr props = NULL;
    drmModePropertyPtr prop = NULL;
    char *buf;
    unsigned int i;
    if (plane_id <= 0)
        return;
    if (drmSetClientCap(self->fd, DRM_CLIENT_CAP_ATOMIC, 1))
        return;
    props = drmModeObjectGetProperties(self->fd, plane_id,
                                       DRM_MODE_OBJECT_PLANE);
    if (!props)
        goto out;
    for (i = 0; i < props->count_props; i++)
    {
        prop = drmModeGetProperty(self->fd, props->props[i]);
        if (prop && !strcmp(prop->name, "ZPOS"))
            break;
        drmModeFreeProperty(prop);
        prop = NULL;
    }
    if (!prop)
        goto out;
    drmModeObjectSetProperty(self->fd, plane_id,
                             DRM_MODE_OBJECT_PLANE, props->props[i], zpos);
out:
    drmModeFreeProperty(prop);
    drmModeFreeObjectProperties(props);
}

static void drm_free(drm_device_t *dev)
{
    if (dev->res)
    {
        drmModeFreeResources(dev->res);
        dev->res = NULL;
    }
    dev->connector_id = 0;
    dev->crtc_id = 0;
    dev->plane_id = 0;
    dev->mode.hdisplay = 0;
    dev->mode.vdisplay = 0;
}

static int drm_setup(drm_device_t *dev)
{
    drmModeConnectorPtr conn = NULL;
    drmModeModeInfoPtr mode;
    drmModePlanePtr plane = NULL;
    drmModeCrtcPtr crtc = NULL;
    drm_bo_t *crtc_bo;
    int ret;
    int i, success = 0;

    dev->res = drmModeGetResources(dev->fd);
    if (!dev->res)
    {
        LV_LOG_ERROR("drm get resource failed");
        goto err;
    }

    conn = drm_find_best_connector(dev);
    if (!conn)
    {
        LV_LOG_ERROR("drm find connector failed");
        goto err;
    }
    LV_LOG_TRACE("Best connector id: %d", conn->connector_id);

    mode = drm_find_best_mode(dev, conn);
    if (!mode)
    {
        LV_LOG_ERROR("drm find mode failed");
        goto err;
    }
    LV_LOG_TRACE("Best mode: %dx%d", mode->hdisplay, mode->vdisplay);

    crtc = drm_find_best_crtc(dev, conn);
    if (!crtc)
    {
        LV_LOG_ERROR("drm find crtc failed");
        goto err;
    }
    LV_LOG_TRACE("Best crtc: %d", crtc->crtc_id);

    plane = drm_find_best_plane(dev, crtc);
    if (!plane)
    {
        LV_LOG_ERROR("drm find plane failed");
        goto err;
    }

    configure_plane_zpos(dev, plane->plane_id, 1);
    LV_LOG_USER("Best plane: %d", plane->plane_id);
    dev->connector_id = conn->connector_id;
    dev->crtc_id = crtc->crtc_id;
    dev->plane_id = plane->plane_id;
    dev->last_fb_id = 0;
    dev->mode.hdisplay = mode->hdisplay;
    dev->mode.vdisplay = mode->vdisplay;

    drm_setup_fb_mode(dev);
    LV_LOG_USER("Drm fb mode: %dx%d", dev->mode.width, dev->mode.height);
    crtc_bo = malloc_drm_bo(dev, dev->mode.width, dev->mode.height,
                            DRM_FORMAT);
    if (crtc_bo)
    {
        memset(crtc_bo->ptr, 0x0, crtc_bo->size);
        ret = drmModeSetCrtc(dev->fd, dev->crtc_id, crtc_bo->fb_id, 0, 0,
                             dev->con_ids, dev->num_cons, &dev->mode_info);
        if (ret)
            LV_LOG_WARN("drmModeSetCrtc failed");
        free_drm_bo(dev, crtc_bo);
    }

    success = 1;
err:
    drmModeFreeConnector(conn);
    drmModeFreePlane(plane);
    drmModeFreeCrtc(crtc);
    if (!success)
    {
        drm_free(dev);
        return -1;
    }
    return 0;
}
static void drm_flip_handler(int fd, unsigned frame, unsigned sec,
                             unsigned usec, void *data)
{
    // data is &dev->waiting_for_flip
    LV_LOG_TRACE("Page flip received(%d)!, %d, %d, %d, %d", *(int *)data, fd, frame, sec,
                 sec);
    *(int *)data = 0;
}

static drm_device_t *drm_init(int bpp)
{
    drm_device_t *dev;
    int ret;

    dev = lv_malloc_zeroed(sizeof(drm_device_t));
    if (dev == NULL)
    {
        LV_LOG_ERROR("allocate device failed");
        return NULL;
    }

    dev->fd = drmOpen("rockchip", NULL);
    if (dev->fd < 0)
        dev->fd = open("/dev/dri/card0", O_RDWR);
    if (dev->fd < 0)
    {
        LV_LOG_ERROR("drm open failed");
        goto err_drm_open;
    }
    fcntl(dev->fd, F_SETFD, FD_CLOEXEC);

    drmSetClientCap(dev->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);

    ret = drm_setup(dev);
    if (ret)
    {
        LV_LOG_ERROR("drm setup failed");
        goto err_drm_setup;
    }

    dev->drm_pollfd.fd = dev->fd;
    dev->drm_pollfd.events = POLLIN;

    dev->drm_evctx.version = DRM_EVENT_CONTEXT_VERSION;
    dev->drm_evctx.page_flip_handler = drm_flip_handler;

    return dev;
err_alloc_fb:
    drm_free(dev);
err_drm_setup:
    drmClose(dev->fd);
err_drm_open:
    lv_free(dev);
    return NULL;
}

static void drm_deinit(drm_device_t *dev)
{
    drm_free(dev);

    if (dev->fd > 0)
        drmClose(dev->fd);
}

static void drm_wait_flip(drm_device_t *dev, int timeout)
{
    int ret;

    while (dev->waiting_for_flip)
    {
        dev->drm_pollfd.revents = 0;
        ret = poll(&dev->drm_pollfd, 1, timeout);
        if (ret <= 0)
            return;
        drmHandleEvent(dev->fd, &dev->drm_evctx);
    }
}

static void setdrmdisp(drm_device_t *dev, drm_bo_t *bo)
{
    if (dev == NULL)
        return;

    int crtc_x, crtc_y, crtc_w, crtc_h;
    int ret;
    int fb = bo->fb_id, sw = dev->mode.width, sh = dev->mode.height;

    crtc_w = dev->mode.width;
    crtc_h = dev->mode.height;
    crtc_x = 0;
    crtc_y = 0;

    LV_LOG_TRACE("Display bo %d(%dx%d) at (%d,%d) %dx%d", fb, sw, sh,
                 crtc_x, crtc_y, crtc_w, crtc_h);
    ret = drmModeSetPlane(dev->fd, dev->plane_id, dev->crtc_id, fb, 0,
                          crtc_x, crtc_y, crtc_w, crtc_h,
                          0, 0, sw << 16, sh << 16);
    if (ret)
    {
        LV_LOG_ERROR("drm set plane failed");
        return;
    }
    if (1)
    {
        // Queue page flip
        dev->waiting_for_flip = 1;
        ret = drmModePageFlip(dev->fd, dev->crtc_id, fb,
                              DRM_MODE_PAGE_FLIP_EVENT, &dev->waiting_for_flip);
        if (ret)
        {
            LV_LOG_ERROR("drm page flip failed");
            return;
        }
        // Wait for last page flip
        drm_wait_flip(dev, -1);
    }
}
static void *drm_thread(void *arg)
{
    drm_device_t *dev = (drm_device_t *)arg;
    drm_bo_t *bo = NULL;
    drm_bo_t *vop_buf[2];
#if LV_DRM_USE_RGA
    drm_bo_t *gbo = dev->gbo;
    rga_buffer_t vop_img[2];
    rga_buffer_t src_img, dst_img, pat_img;
    im_rect src_rect, dst_rect, pat_rect;
    int usage = IM_SYNC;
    int src_fd;
    int dst_fd;
    int ret;
#endif
    vop_buf[0] = dev->vop_buf[0];
    vop_buf[1] = dev->vop_buf[1];
#if LV_DRM_USE_RGA
    switch (dev->disp_rot)
    {
    case 90: usage |= IM_HAL_TRANSFORM_ROT_90; break;
    case 180: usage |= IM_HAL_TRANSFORM_ROT_180; break;
    case 270: usage |= IM_HAL_TRANSFORM_ROT_270; break;
    default: break;
    }
        src_img = wrapbuffer_fd(gbo->buf_fd, gbo->w,
                            gbo->h, RGA_FORMAT,
                            gbo->pitch / (LV_COLOR_DEPTH >> 3),
                            gbo->h);
    vop_img[0] = wrapbuffer_fd(vop_buf[0]->buf_fd, vop_buf[0]->w,
                               vop_buf[0]->h, RGA_FORMAT,
                               vop_buf[0]->pitch / (LV_COLOR_DEPTH >> 3),
                               vop_buf[0]->h);
    vop_img[1] = wrapbuffer_fd(vop_buf[1]->buf_fd, vop_buf[1]->w,
                               vop_buf[1]->h, RGA_FORMAT,
                               vop_buf[1]->pitch / (LV_COLOR_DEPTH >> 3),
                               vop_buf[1]->h);
    memset(&pat_img, 0, sizeof(pat_img));
    memset(&src_rect, 0, sizeof(src_rect));
    memset(&dst_rect, 0, sizeof(dst_rect));
    memset(&pat_rect, 0, sizeof(pat_rect));
#endif
    while (!dev->quit)
    {
        pthread_mutex_lock(&dev->mutex);
        if (dev->draw_update)
        {
            bo = (bo == vop_buf[0]) ? vop_buf[1] : vop_buf[0];

#if LV_DRM_USE_RGA
            dst_img = (bo == vop_buf[0]) ? vop_img[0] : vop_img[1];
            ret = imcheck_composite(src_img, dst_img, pat_img,
                                    src_rect, dst_rect, pat_rect);
            if (ret != IM_STATUS_NOERROR)
            {
                LV_LOG_ERROR("%d, check error! %s\n", __LINE__,
                             imStrError((IM_STATUS)ret));
            }
            else
            {
                ret = improcess(src_img, dst_img, pat_img,
                                src_rect, dst_rect, pat_rect, usage);
                if (ret != IM_STATUS_SUCCESS)
                    LV_LOG_ERROR("%d, running failed, %s\n", __LINE__,
                                 imStrError((IM_STATUS)ret));
            }
#else
            for (int i = 0; i < dev->mode.height; i++)
            {
                memcpy(bo->ptr + i * bo->pitch,
                       dev->drm_buff + i * dev->lcd_sw * (LV_COLOR_DEPTH >> 3),
                       dev->mode.width * (LV_COLOR_DEPTH >> 3));
            }
#endif
            setdrmdisp(dev, bo);
            dev->draw_update = 0;
        }

        pthread_mutex_unlock(&dev->mutex);
        usleep(100);
    }
    return NULL;
}

static void drm_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p)
{
    drm_device_t *dev = lv_display_get_driver_data(disp);
    int32_t x;
    int32_t y;
    int32_t w = lv_area_get_width(area);
    int32_t h = lv_area_get_height(area);

    pthread_mutex_lock(&dev->mutex);

    for (y = area->y1; y <= area->y2; y++)
    {
        lv_color_t *ptr = (lv_color_t *)(dev->drm_buff + (y * dev->lcd_sw + area->x1) * (LV_COLOR_DEPTH >> 3));
        memcpy(ptr, color_p, w * (LV_COLOR_DEPTH >> 3));
        color_p += w * (LV_COLOR_DEPTH >> 3);
    }
    if (lv_display_flush_is_last(disp))
        dev->draw_update = 1;
    pthread_mutex_unlock(&dev->mutex);

    lv_display_flush_ready(disp);
}

static void drm_buffer_setup(drm_device_t *dev)
{
    int buf_w, buf_h;

    LV_LOG_USER("bit depth %d", LV_COLOR_DEPTH);

    if ((dev->disp_rot == 0) || (dev->disp_rot == 180))
    {
        buf_w = dev->mode.width;
        buf_h = dev->mode.height;
    }
    else
    {
        buf_w = dev->mode.height;
        buf_h = dev->mode.width;
    }

    #if LV_DRM_USE_RGA
    dev->gbo = malloc_drm_bo(dev, buf_w, buf_h, DRM_FORMAT);
    dev->drm_buff = dev->gbo->ptr;
    dev->lcd_sw = dev->gbo->pitch / (LV_COLOR_DEPTH >> 3);
    c_RkRgaInit();
#else
    dev->drm_buff = malloc(buf_w * buf_h * (LV_COLOR_DEPTH >> 3));
    dev->lcd_sw = buf_w;
#endif
    dev->vop_buf[0] = malloc_drm_bo(dev, dev->mode.width, dev->mode.height,
                                    DRM_FORMAT);
    dev->vop_buf[1] = malloc_drm_bo(dev, dev->mode.width, dev->mode.height,
                                    DRM_FORMAT);
    LV_LOG_USER("DRM subsystem and buffer mapped successfully");
}

static void drm_buffer_destroy(drm_device_t *dev)
{
#if LV_DRM_USE_RGA
    free_drm_bo(dev, dev->gbo);
#else
    free(dev->drm_buff);
    free(dev->disp_buf);
#endif
    free_drm_bo(dev, dev->vop_buf[0]);
    free_drm_bo(dev, dev->vop_buf[1]);
}

lv_display_t *lv_drm_disp_create(int rot)
{
    lv_display_t *disp;

    drm_device_t *dev = drm_init(32);
    if (!dev)
        return NULL;
#if LV_DRM_USE_RGA
    dev->disp_rot = rot;
#endif
    drm_buffer_setup(dev);
    if ((dev->disp_rot == 0) || (dev->disp_rot == 180))
        disp = lv_display_create(dev->mode.width, dev->mode.height);
    else
        disp = lv_display_create(dev->mode.height, dev->mode.width);

    if (disp == NULL)
    {
        LV_LOG_ERROR("lv_display_create failed");
        return NULL;
    }
    int size = dev->mode.width * dev->mode.height * (LV_COLOR_DEPTH >> 3);
    dev->disp_buf = lv_malloc(size);
    lv_display_set_driver_data(disp, dev);
    lv_display_set_flush_cb(disp, drm_flush);
    lv_display_set_buffers(disp, dev->disp_buf, NULL, size,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    pthread_mutex_init(&dev->mutex, NULL);
    pthread_create(&dev->pid, NULL, drm_thread, dev);
    return disp;
}

int lv_drm_disp_delete(lv_display_t *disp)
{
    drm_device_t *dev = lv_display_get_driver_data(disp);

    if (!dev)
        goto end;
    dev->quit = 0;
    pthread_join(dev->pid, NULL);

    drm_buffer_destroy(dev);

    drm_deinit(dev);

    lv_free(dev->disp_buf);
    lv_free(dev);
end:
    lv_display_delete(disp);

    return 0;
}

// static uint32_t tick_get_cb(void)
// {
//     struct timespec t;
//     clock_gettime(CLOCK_MONOTONIC, &t);
//     uint64_t time_ms = t.tv_sec * 1000 + (t.tv_nsec / 1000000);
//     return time_ms;
// }

#endif /*LV_USE_LINUX_DRM*/
