#ifndef __KALMAN_FILTER_H__
#define __KALMAN_FILTER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    double v;
    double P;
    double Q;
    double R;
    double K;
} KalmanFilter;

void KalmanInit(KalmanFilter *kf, double initial_state, double initial_error,
                double process_noise, double measurement_noise);
void KalmanUpdate(KalmanFilter *kf, double measurement, int first);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif

