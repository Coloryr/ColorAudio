/*
 *
 *  Aic Bluetooth USB driver
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/skbuff.h>
#include <linux/errno.h>
#include <linux/usb.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/poll.h>

#include <linux/version.h>
#include <linux/pm_runtime.h>
#include <linux/firmware.h>
#include <linux/suspend.h>


#ifdef CONFIG_PLATFORM_UBUNTU
#define CONFIG_BLUEDROID        0 /* bleuz 0, bluedroid 1 */
#else
#define CONFIG_BLUEDROID        0 /* bleuz 0, bluedroid 1 */
#endif


//#define CONFIG_SCO_OVER_HCI
#define CONFIG_USB_AIC_UART_SCO_DRIVER
//#define CONFIG_BT_WAKEUP_IN_PM

#ifdef CONFIG_SCO_OVER_HCI
#include <linux/usb/audio.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>

#define AIC_SCO_ID "snd_sco_aic"
enum {
	USB_CAPTURE_RUNNING,
	USB_PLAYBACK_RUNNING,
	ALSA_CAPTURE_OPEN,
	ALSA_PLAYBACK_OPEN,
	ALSA_CAPTURE_RUNNING,
	ALSA_PLAYBACK_RUNNING,
	CAPTURE_URB_COMPLETED,
	PLAYBACK_URB_COMPLETED,
	DISCONNECTED,
};

// AIC sound card
typedef struct AIC_sco_card {
    struct snd_card *card;
    struct snd_pcm *pcm;
    struct usb_device *dev;
    struct btusb_data *usb_data;
    unsigned long states;
    struct aic_sco_stream {
		    struct snd_pcm_substream *substream;
		    unsigned int sco_packet_bytes;
		    snd_pcm_uframes_t buffer_pos;
	  } capture, playback;
    spinlock_t capture_lock;
    spinlock_t playback_lock;
    struct work_struct send_sco_work;
} AIC_sco_card_t;
#endif
/* Some Android system may use standard Linux kernel, while
 * standard Linux may also implement early suspend feature.
 * So exclude earysuspend.h from CONFIG_BLUEDROID.
 */
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#if CONFIG_BLUEDROID
#else
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#include <net/bluetooth/hci.h>
#endif


/***********************************
** AicSemi - For aic_btusb driver **
***********************************/
#define URB_CANCELING_DELAY_MS          10
/* when OS suspended, module is still powered,usb is not powered,
 * this may set to 1, and must comply with special patch code.
 */
#define CONFIG_RESET_RESUME     1
#define PRINT_CMD_EVENT         0
#define PRINT_ACL_DATA          0
#define PRINT_SCO_DATA          0

#define AICBT_DBG_FLAG          0

#if AICBT_DBG_FLAG
#define AICBT_DBG(fmt, arg...) printk( "aic_btusb: " fmt "\n" , ## arg)
#else
#define AICBT_DBG(fmt, arg...)
#endif

#define AICBT_INFO(fmt, arg...) printk("aic_btusb: " fmt "\n" , ## arg)
#define AICBT_WARN(fmt, arg...) printk("aic_btusb: " fmt "\n" , ## arg)
#define AICBT_ERR(fmt, arg...) printk("aic_btusb: " fmt "\n" , ## arg)


#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 33)
#define HDEV_BUS        hdev->bus
#define USB_RPM            1
#else
#define HDEV_BUS        hdev->type
#define USB_RPM            0
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 38)
#define NUM_REASSEMBLY 3
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 4, 0)
#define GET_DRV_DATA(x)        hci_get_drvdata(x)
#else
#define GET_DRV_DATA(x)        x->driver_data
#endif

#define SCO_NUM    hdev->conn_hash.sco_num


#define BTUSB_RPM        (0 * USB_RPM) /* 1 SS enable; 0 SS disable */
#define BTUSB_WAKEUP_HOST        0    /* 1  enable; 0  disable */
#define BTUSB_MAX_ISOC_FRAMES    48
#define BTUSB_INTR_RUNNING        0
#define BTUSB_BULK_RUNNING        1
#define BTUSB_ISOC_RUNNING        2
#define BTUSB_SUSPENDING        3
#define BTUSB_DID_ISO_RESUME    4

#define HCI_VENDOR_USB_DISC_HARDWARE_ERROR   0xFF
#define HCI_VENDOR_USB_RESUME_HARDWARE_ERROR   0xFE

#define HCI_CMD_READ_BD_ADDR 0x1009
#define HCI_VENDOR_READ_LMP_VERISION 0x1001
#define HCI_VENDOR_RESET                       0x0C03

#define DRV_NORMAL_MODE 0
#define DRV_MP_MODE 1
int mp_drv_mode = 0; /* 1 Mptool Fw; 0 Normal Fw */

#if CONFIG_BLUEDROID
/* -----  HCI Commands ---- */
#define HCI_OP_INQUIRY            0x0401
#define HCI_OP_INQUIRY_CANCEL        0x0402
#define HCI_OP_EXIT_PERIODIC_INQ    0x0404
#define HCI_OP_CREATE_CONN        0x0405
#define HCI_OP_DISCONNECT                0x0406
#define HCI_OP_ADD_SCO            0x0407
#define HCI_OP_CREATE_CONN_CANCEL    0x0408
#define HCI_OP_ACCEPT_CONN_REQ        0x0409
#define HCI_OP_REJECT_CONN_REQ        0x040a
#define HCI_OP_LINK_KEY_REPLY        0x040b
#define HCI_OP_LINK_KEY_NEG_REPLY    0x040c
#define HCI_OP_PIN_CODE_REPLY        0x040d
#define HCI_OP_PIN_CODE_NEG_REPLY    0x040e
#define HCI_OP_CHANGE_CONN_PTYPE    0x040f
#define HCI_OP_AUTH_REQUESTED        0x0411
#define HCI_OP_SET_CONN_ENCRYPT        0x0413
#define HCI_OP_CHANGE_CONN_LINK_KEY    0x0415
#define HCI_OP_REMOTE_NAME_REQ        0x0419
#define HCI_OP_REMOTE_NAME_REQ_CANCEL    0x041a
#define HCI_OP_READ_REMOTE_FEATURES    0x041b
#define HCI_OP_READ_REMOTE_EXT_FEATURES    0x041c
#define HCI_OP_READ_REMOTE_VERSION    0x041d
#define HCI_OP_SETUP_SYNC_CONN        0x0428
#define HCI_OP_ACCEPT_SYNC_CONN_REQ    0x0429
#define HCI_OP_REJECT_SYNC_CONN_REQ    0x042a
#define HCI_OP_SNIFF_MODE        0x0803
#define HCI_OP_EXIT_SNIFF_MODE        0x0804
#define HCI_OP_ROLE_DISCOVERY        0x0809
#define HCI_OP_SWITCH_ROLE        0x080b
#define HCI_OP_READ_LINK_POLICY        0x080c
#define HCI_OP_WRITE_LINK_POLICY    0x080d
#define HCI_OP_READ_DEF_LINK_POLICY    0x080e
#define HCI_OP_WRITE_DEF_LINK_POLICY    0x080f
#define HCI_OP_SNIFF_SUBRATE        0x0811
#define HCI_OP_Write_Link_Policy_Settings 0x080d
#define HCI_OP_SET_EVENT_MASK        0x0c01
#define HCI_OP_RESET            0x0c03
#define HCI_OP_SET_EVENT_FLT        0x0c05
#endif

#define HCI_OP_Write_Extended_Inquiry_Response        0x0c52
#define HCI_OP_Write_Simple_Pairing_Mode 0x0c56
#define HCI_OP_Read_Buffer_Size 0x1005
#define HCI_OP_Host_Buffer_Size 0x0c33
#define HCI_OP_Read_Local_Version_Information 0x1001
#define HCI_OP_Read_BD_ADDR 0x1009
#define HCI_OP_Read_Local_Supported_Commands 0x1002
#define HCI_OP_Write_Scan_Enable 0x0c1a
#define HCI_OP_Write_Current_IAC_LAP 0x0c3a
#define HCI_OP_Write_Inquiry_Scan_Activity 0x0c1e
#define HCI_OP_Write_Class_of_Device 0x0c24
#define HCI_OP_LE_Rand 0x2018
#define HCI_OP_LE_Set_Random_Address 0x2005
#define HCI_OP_LE_Set_Extended_Scan_Enable 0x2042
#define HCI_OP_LE_Set_Extended_Scan_Parameters 0x2041
#define HCI_OP_Set_Event_Filter 0x0c05
#define HCI_OP_Write_Voice_Setting 0x0c26
#define HCI_OP_Change_Local_Name 0x0c13
#define HCI_OP_Read_Local_Name 0x0c14
#define HCI_OP_Wirte_Page_Timeout 0x0c18
#define HCI_OP_LE_Clear_Resolving_List 0x0c29
#define HCI_OP_LE_Set_Addres_Resolution_Enable_Command 0x0c2e
#define HCI_OP_Write_Inquiry_mode 0x0c45
#define HCI_OP_Write_Page_Scan_Type 0x0c47
#define HCI_OP_Write_Inquiry_Scan_Type 0x0c43



#define HCI_OP_Delete_Stored_Link_Key 0x0c12
#define HCI_OP_LE_Read_Local_Resolvable_Address 0x202d
#define HCI_OP_LE_Extended_Create_Connection 0x2043
#define HCI_OP_Read_Remote_Version_Information 0x041d
#define HCI_OP_LE_Start_Encryption 0x2019
#define HCI_OP_LE_Add_Device_to_Resolving_List 0x2027
#define HCI_OP_LE_Set_Privacy_Mode 0x204e
#define HCI_OP_LE_Connection_Update 0x2013

#define HCI_OP_LE_Read_Local_Support_Featrues 0x2003 
#define HCI_OP_LE_Get_Vendor_Capabilities_Command   0xfd53
#define HCI_OP_LE_Set_Le_Scan_Enable 0x200C
#define HCI_OP_LE_Create_Connection 0x200d


#if CONFIG_BLUEDROID
/* -----  HCI events---- */
#define HCI_OP_DISCONNECT        0x0406
#define HCI_EV_INQUIRY_COMPLETE        0x01
#define HCI_EV_INQUIRY_RESULT        0x02
#define HCI_EV_CONN_COMPLETE        0x03
#define HCI_EV_CONN_REQUEST            0x04
#define HCI_EV_DISCONN_COMPLETE        0x05
#define HCI_EV_AUTH_COMPLETE        0x06
#define HCI_EV_REMOTE_NAME            0x07
#define HCI_EV_ENCRYPT_CHANGE        0x08
#define HCI_EV_CHANGE_LINK_KEY_COMPLETE    0x09

#define HCI_EV_REMOTE_FEATURES        0x0b
#define HCI_EV_REMOTE_VERSION        0x0c
#define HCI_EV_QOS_SETUP_COMPLETE    0x0d
#define HCI_EV_CMD_COMPLETE            0x0e
#define HCI_EV_CMD_STATUS            0x0f

#define HCI_EV_ROLE_CHANGE            0x12
#define HCI_EV_NUM_COMP_PKTS        0x13
#define HCI_EV_MODE_CHANGE            0x14
#define HCI_EV_PIN_CODE_REQ            0x16
#define HCI_EV_LINK_KEY_REQ            0x17
#define HCI_EV_LINK_KEY_NOTIFY        0x18
#define HCI_EV_CLOCK_OFFSET            0x1c
#define HCI_EV_PKT_TYPE_CHANGE        0x1d
#define HCI_EV_PSCAN_REP_MODE        0x20

#define HCI_EV_INQUIRY_RESULT_WITH_RSSI    0x22
#define HCI_EV_REMOTE_EXT_FEATURES    0x23
#define HCI_EV_SYNC_CONN_COMPLETE    0x2c
#define HCI_EV_SYNC_CONN_CHANGED    0x2d
#define HCI_EV_SNIFF_SUBRATE            0x2e
#define HCI_EV_EXTENDED_INQUIRY_RESULT    0x2f
#define HCI_EV_IO_CAPA_REQUEST        0x31
#define HCI_EV_SIMPLE_PAIR_COMPLETE    0x36
#define HCI_EV_REMOTE_HOST_FEATURES    0x3d
#define HCI_EV_LE_Meta 0x3e

/* ULP Event sub code */
#define HCI_BLE_CONN_COMPLETE_EVT 0x01
#define HCI_BLE_ADV_PKT_RPT_EVT 0x02
#define HCI_BLE_LL_CONN_PARAM_UPD_EVT 0x03
#define HCI_BLE_READ_REMOTE_FEAT_CMPL_EVT 0x04
#define HCI_BLE_LTK_REQ_EVT 0x05
#define HCI_BLE_RC_PARAM_REQ_EVT 0x06
#define HCI_BLE_DATA_LENGTH_CHANGE_EVT 0x07
#define HCI_BLE_ENHANCED_CONN_COMPLETE_EVT 0x0a
#define HCI_BLE_DIRECT_ADV_EVT 0x0b
#define HCI_BLE_PHY_UPDATE_COMPLETE_EVT 0x0c
#define HCI_LE_EXTENDED_ADVERTISING_REPORT_EVT 0x0D
#define HCI_BLE_PERIODIC_ADV_SYNC_EST_EVT      0x0E
#define HCI_BLE_PERIODIC_ADV_REPORT_EVT        0x0F
#define HCI_BLE_PERIODIC_ADV_SYNC_LOST_EVT     0x10
#define HCI_BLE_SCAN_TIMEOUT_EVT               0x11
#define HCI_LE_ADVERTISING_SET_TERMINATED_EVT 0x12
#define HCI_BLE_SCAN_REQ_RX_EVT                0x13
#define HCI_BLE_CIS_EST_EVT 0x19
#define HCI_BLE_CIS_REQ_EVT 0x1a
#define HCI_BLE_CREATE_BIG_CPL_EVT 0x1b
#define HCI_BLE_TERM_BIG_CPL_EVT 0x1c
#define HCI_BLE_BIG_SYNC_EST_EVT 0x1d
#define HCI_BLE_BIG_SYNC_LOST_EVT 0x1e
#define HCI_BLE_REQ_PEER_SCA_CPL_EVT 0x1f

#define HCI_VENDOR_SPECIFIC_EVT 0xFF /* Vendor specific events */
#endif

#if CONFIG_BLUEDROID
#define QUEUE_SIZE 500

/***************************************
** AicSemi - Integrate from bluetooth.h **
*****************************************/
/* Reserv for core and drivers use */
#define BT_SKB_RESERVE    8

/* BD Address */
typedef struct {
    __u8 b[6];
} __packed bdaddr_t;

/* Skb helpers */
struct bt_skb_cb {
    __u8 pkt_type;
    __u8 incoming;
    __u16 expect;
    __u16 tx_seq;
    __u8 retries;
    __u8 sar;
    __u8 force_active;
};

#define bt_cb(skb) ((struct bt_skb_cb *)((skb)->cb))

static inline struct sk_buff *bt_skb_alloc(unsigned int len, gfp_t how)
{
    struct sk_buff *skb;

    if ((skb = alloc_skb(len + BT_SKB_RESERVE, how))) {
        skb_reserve(skb, BT_SKB_RESERVE);
        bt_cb(skb)->incoming  = 0;
    }
    return skb;
}
/* AicSemi - Integrate from bluetooth.h end */

/***********************************
** AicSemi - Integrate from hci.h **
***********************************/
#define HCI_MAX_ACL_SIZE    1024
#define HCI_MAX_SCO_SIZE    255
#define HCI_MAX_EVENT_SIZE    260
#define HCI_MAX_FRAME_SIZE    (HCI_MAX_ACL_SIZE + 4)

/* HCI bus types */
#define HCI_VIRTUAL    0
#define HCI_USB        1
#define HCI_PCCARD    2
#define HCI_UART    3
#define HCI_RS232    4
#define HCI_PCI        5
#define HCI_SDIO    6

/* HCI controller types */
#define HCI_BREDR    0x00
#define HCI_AMP        0x01

/* HCI device flags */
enum {
    HCI_UP,
    HCI_INIT,
    HCI_RUNNING,

    HCI_PSCAN,
    HCI_ISCAN,
    HCI_AUTH,
    HCI_ENCRYPT,
    HCI_INQUIRY,

    HCI_RAW,

    HCI_RESET,
};

/*
 * BR/EDR and/or LE controller flags: the flags defined here should represent
 * states from the controller.
 */
enum {
    HCI_SETUP,
    HCI_AUTO_OFF,
    HCI_MGMT,
    HCI_PAIRABLE,
    HCI_SERVICE_CACHE,
    HCI_LINK_KEYS,
    HCI_DEBUG_KEYS,
    HCI_UNREGISTER,

    HCI_LE_SCAN,
    HCI_SSP_ENABLED,
    HCI_HS_ENABLED,
    HCI_LE_ENABLED,
    HCI_CONNECTABLE,
    HCI_DISCOVERABLE,
    HCI_LINK_SECURITY,
    HCI_PENDING_CLASS,
};

/* HCI data types */
#define HCI_COMMAND_PKT        0x01
#define HCI_ACLDATA_PKT        0x02
#define HCI_SCODATA_PKT        0x03
#define HCI_EVENT_PKT        0x04
#define HCI_VENDOR_PKT        0xff

#define HCI_MAX_NAME_LENGTH        248
#define HCI_MAX_EIR_LENGTH        240

#define HCI_OP_READ_LOCAL_VERSION    0x1001
struct hci_rp_read_local_version {
    __u8     status;
    __u8     hci_ver;
    __le16   hci_rev;
    __u8     lmp_ver;
    __le16   manufacturer;
    __le16   lmp_subver;
} __packed;

#define HCI_EV_CMD_COMPLETE        0x0e
struct hci_ev_cmd_complete {
    __u8     ncmd;
    __le16   opcode;
} __packed;

/* ---- HCI Packet structures ---- */
#define HCI_COMMAND_HDR_SIZE 3
#define HCI_EVENT_HDR_SIZE   2
#define HCI_ACL_HDR_SIZE     4
#define HCI_SCO_HDR_SIZE     3

struct hci_command_hdr {
    __le16    opcode;        /* OCF & OGF */
    __u8    plen;
} __packed;

struct hci_event_hdr {
    __u8    evt;
    __u8    plen;
} __packed;

struct hci_acl_hdr {
    __le16    handle;        /* Handle & Flags(PB, BC) */
    __le16    dlen;
} __packed;

struct hci_sco_hdr {
    __le16    handle;
    __u8    dlen;
} __packed;

static inline struct hci_event_hdr *hci_event_hdr(const struct sk_buff *skb)
{
    return (struct hci_event_hdr *) skb->data;
}

static inline struct hci_acl_hdr *hci_acl_hdr(const struct sk_buff *skb)
{
    return (struct hci_acl_hdr *) skb->data;
}

static inline struct hci_sco_hdr *hci_sco_hdr(const struct sk_buff *skb)
{
    return (struct hci_sco_hdr *) skb->data;
}

/* ---- HCI Ioctl requests structures ---- */
struct hci_dev_stats {
    __u32 err_rx;
    __u32 err_tx;
    __u32 cmd_tx;
    __u32 evt_rx;
    __u32 acl_tx;
    __u32 acl_rx;
    __u32 sco_tx;
    __u32 sco_rx;
    __u32 byte_rx;
    __u32 byte_tx;
};
/* AicSemi - Integrate from hci.h end */

/*****************************************
** AicSemi - Integrate from hci_core.h  **
*****************************************/
struct hci_conn_hash {
    struct list_head list;
    unsigned int     acl_num;
    unsigned int     sco_num;
    unsigned int     le_num;
};

#define HCI_MAX_SHORT_NAME_LENGTH    10

#define NUM_REASSEMBLY 4
struct hci_dev {
    struct mutex    lock;

    char        name[8];
    unsigned long    flags;
    __u16        id;
    __u8        bus;
    __u8        dev_type;

    struct sk_buff        *reassembly[NUM_REASSEMBLY];

    struct hci_conn_hash    conn_hash;

    struct hci_dev_stats    stat;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 4, 0)
    atomic_t        refcnt;
    struct module           *owner;
    void                    *driver_data;
#endif

    atomic_t        promisc;

    struct device        *parent;
    struct device        dev;

    unsigned long        dev_flags;

    int (*open)(struct hci_dev *hdev);
    int (*close)(struct hci_dev *hdev);
    int (*flush)(struct hci_dev *hdev);
    int (*send)(struct sk_buff *skb);
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 4, 0)
    void (*destruct)(struct hci_dev *hdev);
#endif
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 7, 1)
    __u16               voice_setting;
#endif
    void (*notify)(struct hci_dev *hdev, unsigned int evt);
    int (*ioctl)(struct hci_dev *hdev, unsigned int cmd, unsigned long arg);
	u8 *align_data;
};

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 4, 0)
static inline struct hci_dev *__hci_dev_hold(struct hci_dev *d)
{
    atomic_inc(&d->refcnt);
    return d;
}

static inline void __hci_dev_put(struct hci_dev *d)
{
    if (atomic_dec_and_test(&d->refcnt))
        d->destruct(d);
}
#endif

static inline void *hci_get_drvdata(struct hci_dev *hdev)
{
    return dev_get_drvdata(&hdev->dev);
}

static inline void hci_set_drvdata(struct hci_dev *hdev, void *data)
{
    dev_set_drvdata(&hdev->dev, data);
}

#define SET_HCIDEV_DEV(hdev, pdev) ((hdev)->parent = (pdev))
/* AicSemi - Integrate from hci_core.h end */


#define CONFIG_MAC_OFFSET_GEN_1_2       (0x3C)      //MAC's OFFSET in config/efuse for aic generation 1~2 bluetooth chip
#define CONFIG_MAC_OFFSET_GEN_3PLUS     (0x44)      //MAC's OFFSET in config/efuse for aic generation 3+ bluetooth chip


typedef struct {
    uint16_t    vid;
    uint16_t    pid;
    uint16_t    lmp_sub_default;
    uint16_t    lmp_sub;
    uint16_t    eversion;
    char        *mp_patch_name;
    char        *patch_name;
    char        *config_name;
    uint8_t     *fw_cache;
    int         fw_len;
    uint16_t    mac_offset;
    uint32_t    max_patch_size;
} patch_info;

//Define ioctl cmd the same as HCIDEVUP in the kernel
#define DOWN_FW_CFG             _IOW('E', 176, int)
//#ifdef CONFIG_SCO_OVER_HCI
//#define SET_ISO_CFG             _IOW('H', 202, int)
//#else
#define SET_ISO_CFG             _IOW('E', 177, int)
//#endif
#define RESET_CONTROLLER        _IOW('E', 178, int)
#define DWFW_CMPLT              _IOW('E', 179, int)

#define GET_USB_INFO            _IOR('E', 180, int)

/*  for altsettings*/
#include <linux/fs.h>
#define BDADDR_FILE "/data/misc/bluetooth/bdaddr"
#define FACTORY_BT_BDADDR_STORAGE_LEN 17
#if 0
static inline int getmacaddr(uint8_t * vnd_local_bd_addr)
{
    struct file  *bdaddr_file;
    mm_segment_t oldfs;
    char buf[FACTORY_BT_BDADDR_STORAGE_LEN];
    int32_t i = 0;
    memset(buf, 0, FACTORY_BT_BDADDR_STORAGE_LEN);
    bdaddr_file = filp_open(BDADDR_FILE, O_RDONLY, 0);
    if (IS_ERR(bdaddr_file)){
        AICBT_INFO("No Mac Config for BT\n");
        return -1;
    }
    oldfs = get_fs(); 
    set_fs(KERNEL_DS);
    bdaddr_file->f_op->llseek(bdaddr_file, 0, 0);
    bdaddr_file->f_op->read(bdaddr_file, buf, FACTORY_BT_BDADDR_STORAGE_LEN, &bdaddr_file->f_pos);
    for (i = 0; i < 6; i++) {
     if(buf[3*i]>'9')
     {
         if(buf[3*i]>'Z')
              buf[3*i] -=('a'-'A'); //change  a to A
         buf[3*i] -= ('A'-'9'-1);
     }
     if(buf[3*i+1]>'9')
     {
        if(buf[3*i+1]>'Z')
              buf[3*i+1] -=('a'-'A'); //change  a to A
         buf[3*i+1] -= ('A'-'9'-1);
     }
     vnd_local_bd_addr[5-i] = ((uint8_t)buf[3*i]-'0')*16 + ((uint8_t)buf[3*i+1]-'0');
    }
    set_fs(oldfs);
    filp_close(bdaddr_file, NULL);
    return 0;
}
#endif

#endif /* CONFIG_BLUEDROID */


typedef struct {
    struct usb_interface    *intf;
    struct usb_device        *udev;
    int            pipe_in, pipe_out;
    uint8_t        *send_pkt;
    uint8_t        *rcv_pkt;
    struct hci_command_hdr        *cmd_hdr;
    struct hci_event_hdr        *evt_hdr;
    struct hci_ev_cmd_complete    *cmd_cmp;
    uint8_t        *req_para,    *rsp_para;
    uint8_t        *fw_data;
    int            pkt_len;
    int            fw_len;
} firmware_info;

/*******************************
**    Reasil patch code
********************************/
#define CMD_CMP_EVT        0x0e
#define RCV_PKT_LEN            64
#define SEND_PKT_LEN       300
#define MSG_TO            1000
#define PATCH_SEG_MAX    252
#define DATA_END        0x80
#define DOWNLOAD_OPCODE    0xfc02
#define HCI_VSC_UPDATE_PT_CMD                   0xFC75
#define HCI_VSC_SET_ADFILTER_PT_CMD             0xFDAB
#define HCI_VSC_RESET_ADFILTER_PROCESS_PT_CMD   0xFDAC
#define TRUE            1
#define FALSE            0
#define CMD_HDR_LEN        sizeof(struct hci_command_hdr)
#define EVT_HDR_LEN        sizeof(struct hci_event_hdr)
#define CMD_CMP_LEN        sizeof(struct hci_ev_cmd_complete)
#define MAX_PATCH_SIZE_24K (1024*24)
#define MAX_PATCH_SIZE_40K (1024*40)


#define FW_RAM_ADID_BASE_ADDR           0x101788
#define FW_RAM_PATCH_BASE_ADDR          0x184000
#define AICBT_PT_TAG                    "AICBT_PT_TAG"

enum aicbt_patch_table_type {
	AICBT_PT_INF  = 0x00,
	AICBT_PT_TRAP = 0x1,
	AICBT_PT_B4,
	AICBT_PT_BTMODE,
	AICBT_PT_PWRON,
	AICBT_PT_AF,
	AICBT_PT_VER,
	AICBT_PT_MAX,
};

enum AIC_DC_SUBID{
    DC_U01 = 0,
    DC_U02,
    DC_U02H,
};

struct aicbt_firmware {
	const char *desc;
	const char *bt_adid;
	const char *bt_patch;
	const char *bt_table;
    const char *bt_ext_patch;
};

const struct aicbt_firmware fw_8800dc[] = {
	[DC_U01] = {
        .desc          = "aic8800dc u01 bt patch",
        .bt_adid       = "fw_adid_8800dc.bin",
        .bt_patch      = "fw_patch_8800dc.bin",
        .bt_table      = "fw_patch_table_8800dc.bin",
        .bt_ext_patch  = "fw_patch_8800dc_ext"
	},
    [DC_U02] = {
        .desc          = "aic8800dc u02 bt patch",
        .bt_adid       = "fw_adid_8800dc_u02.bin",
        .bt_patch      = "fw_patch_8800dc_u02.bin",
        .bt_table      = "fw_patch_table_8800dc_u02.bin",
        .bt_ext_patch  = "fw_patch_8800dc_u02_ext"
	},
    [DC_U02H] = {
        .desc          = "aic8800dch u02 bt patch",
        .bt_adid       = "fw_adid_8800dc_u02h.bin",
        .bt_patch      = "fw_patch_8800dc_u02h.bin",
        .bt_table      = "fw_patch_table_8800dc_u02h.bin",
        .bt_ext_patch  = "fw_patch_8800dc_u02h_ext"
    },
};


#define HCI_VSC_FW_STATUS_GET_CMD          0xFC78

struct fw_status {
    u8 status;
} __packed;

#define HCI_PATCH_DATA_MAX_LEN              240
#define HCI_VSC_MEM_WR_SIZE                 240
#define HCI_VSC_MEM_RD_SIZE                 128
#define HCI_VSC_UPDATE_PT_SIZE              249
#define HCI_PT_MAX_LEN                      31

#define HCI_VSC_DBG_RD_MEM_CMD              0xFC01

#define MAX_AD_FILTER_NUM        4// Max AD Filter num
#define MAX_GPIO_TRIGGER_NUM     2// Max user config num of gpio
#define MAX_ROLE_COMNO_IDX_NUM   2// Max num of ad role type combo,form( enum gpio_combo_idx) 

#define AD_ROLE_FLAG         0x0f
#define ROLE_COMBO_IDX_FLAG  0xf0

enum ad_role_type {
    ROLE_ONLY,// ROLE_ONLY will trigger wake up immediately.
    ROLE_COMBO,//ROLE_COMBO will trigger When all the conditions (ad_role == ROLE_COMBO,and ad_filter is matching)are met.
};

enum gpio_combo_idx {
    COMBO_0,
    COMBO_1,
};

enum gpio_trigger_bit {
    TG_IDX_0 = (1<<0),
    TG_IDX_1 = (1<<1),
};

struct wakeup_ad_data_filter {
    uint32_t ad_data_mask;
    uint8_t gpio_trigger_idx;
    uint8_t ad_role;//from enum ad_role_type 
    uint8_t ad_len;
    uint8_t ad_type;
    uint8_t ad_data[31];
	bdaddr_t wl_addr;
};

struct ble_wakeup_param_t {
    uint32_t magic_num;// "BLES" = 0x53454C42
    uint32_t delay_scan_to;// timeout for start scan in ms
    uint32_t reboot_to;// timeout for reboot in ms
    uint32_t gpio_num[MAX_GPIO_TRIGGER_NUM];
    uint32_t gpio_dft_lvl[MAX_GPIO_TRIGGER_NUM];
    struct wakeup_ad_data_filter ad_filter[MAX_AD_FILTER_NUM];
};

struct hci_dbg_rd_mem_cmd {
    __le32 start_addr;
    __u8 type;
    __u8 length;
}__attribute__ ((packed));

struct hci_dbg_rd_mem_cmd_evt {
    __u8 status;
    __u8 length;
    __u8 data[HCI_VSC_MEM_RD_SIZE];
}__attribute__ ((packed));

struct long_buffer_tag {
    __u8 length;
    __u8 data[HCI_VSC_MEM_WR_SIZE];
};

struct hci_dbg_wr_mem_cmd {
    __le32 start_addr;
    __u8 type;
    __u8 length;
    __u8 data[HCI_VSC_MEM_WR_SIZE];
};

struct aicbt_patch_table {
    char     *name;
    uint32_t type;
    uint32_t *data;
    uint32_t len;
    struct aicbt_patch_table *next;
};

struct aicbt_patch_info_t {
    uint32_t info_len;
//base len start
    uint32_t adid_addrinf;
    uint32_t addr_adid;
    uint32_t patch_addrinf;
    uint32_t addr_patch;
    uint32_t reset_addr;
    uint32_t reset_val;
    uint32_t adid_flag_addr;
    uint32_t adid_flag;
//base len end
//ext patch nb
    uint32_t ext_patch_nb_addr;
    uint32_t ext_patch_nb;
    uint32_t *ext_patch_param;
};


struct aicbt_patch_table_cmd {
    uint8_t patch_num;
    uint32_t patch_table_addr[31];
    uint32_t patch_table_data[31];
}__attribute__ ((packed));


enum aic_endpoit {
    CTRL_EP = 0,
    INTR_EP = 3,
    BULK_EP = 1,
    ISOC_EP = 4
};

/* #define HCI_VERSION_CODE KERNEL_VERSION(3, 14, 41) */
#define HCI_VERSION_CODE LINUX_VERSION_CODE

int aic_load_firmware(u8 ** fw_buf, const char *name, struct device *device);
int aicbt_patch_table_free(struct aicbt_patch_table **head);
int download_patch(firmware_info *fw_info, int cached);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 38)
#define NUM_REASSEMBLY 3
#else
#define NUM_REASSEMBLY 4
#endif

