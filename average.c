#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>


#define DEBUG           0


#define SIZE            (6*1024*1024*1024llu)
#define SUM_STEP_SIMD   (8 * 64 * 1024)
#define VAR_STEP        (1024*1024*1024llu)
#define TEST_COUNT      1000
#define GRADIENT        0.8
#define OFFSET          14.5

#define MAX_TOTAL_SIMD_COUNT  (8 * 64 *1024)

/* count should be multiple of 8 */
/* maximum count without total overflow is 8 * ( 1 << 16) = 512 * 1024 */
uint64_t total_simd(uint16_t *data, unsigned count);

void cal_variance_covariance_simd(uint16_t *data_x, 
    uint16_t *data_y,
    uint32_t means,
    uint32_t count,
    int64_t *variance,
    int64_t *covariance);

uint64_t total_c(uint16_t *data,
    uint64_t count)
{
  uint64_t i, total = 0;

  for (i=0;i<count;i++) {
    total += data[i];
  }
  return total;
}

void cal_variance_covariance_c(uint16_t *data_x, 
    uint16_t *data_y,
    uint32_t means,
    uint32_t count,
    int64_t *variance,
    int64_t *covariance)
{
  uint32_t i;
  uint16_t mean_x, mean_y;
  uint32_t result;
  int64_t var = 0;
  int64_t covar = 0;

  mean_x = means & 0xffff;
  mean_y = (means >> 16) & 0xffff;

  for (i=0; i<count; i++) {
    result = ((int)data_x[i] - (int)mean_x) * ((int)data_x[i] - (int)mean_x);
    var += result;
    covar += ((int)data_x[i] - (int)mean_x) * ((int)data_y[i] - (int)mean_y);
  }
  *variance = var;
  *covariance = covar;
}

void find_linear_regression_c(uint16_t *data_x,
    uint16_t *data_y,
    uint64_t count,
    float *m,
    float *c)
{
  uint64_t sum_x, sum_y;
  uint16_t mean_x, mean_y;
  uint32_t mean;
  int64_t var, covar;
  double  var_d = 0, covar_d = 0;

  uint64_t pos, remain;
  uint32_t step;

  printf("%s()\n", __func__);

  sum_x = total_c(data_x, count);
  mean_x = (sum_x + count/2) / count;

  sum_y = total_c(data_y, count);
  mean_y = (sum_y + count/2) / count;

#if (DEBUG == 1)
  printf("sum_x %lu, sum_y %lu, mean_x %d, mean_y %d\n", sum_x, sum_y, mean_x, mean_y);
#endif

  mean = mean_y;
  mean = (mean << 16) | mean_x;

  pos = 0;
  remain = count;

  while (remain) {
    if (remain > VAR_STEP)
      step = VAR_STEP;
    else
      step = remain;

    cal_variance_covariance_c(&data_x[pos], &data_y[pos], mean, step, &var, &covar);
#if (DEBUG == 1)
    printf("var %lu, covar %lu\n", var, covar);
#endif
    pos += step;
    remain -= step;
    var_d += var;
    covar_d += covar;
  }

  *m = covar_d/var_d;
  *c = (double)(mean_y) - *m * (double)mean_x;
}

void find_linear_regression_simd(uint16_t *data_x,
    uint16_t *data_y,
    uint64_t count,
    float *m,
    float *c)
{
  uint64_t sum_x = 0, sum_y = 0;
  uint16_t mean_x, mean_y;
  uint32_t mean;
  int64_t var, covar;
  double  var_d = 0, covar_d = 0;

  uint64_t pos, remain;
  uint32_t step;

  printf("%s()\n", __func__);

  pos = 0;
  remain = count;

  while (remain) {
    if (remain > SUM_STEP_SIMD)
      step = SUM_STEP_SIMD;
    else
      step = remain;

    sum_x += total_simd(&data_x[pos], step);
    pos += step;
    remain -= step;
  }
  mean_x = (sum_x + count/2) / count;

  pos = 0;
  remain = count;

  while (remain) {
    if (remain > SUM_STEP_SIMD)
      step = SUM_STEP_SIMD;
    else
      step = remain;

    sum_y += total_simd(&data_y[pos], step);
    pos += step;
    remain -= step;
  }
  mean_y = (sum_y + count/2) / count;

#if (DEBUG == 1)
  printf("sum_x %lu, sum_y %lu, mean_x %d, mean_y %d\n", sum_x, sum_y, mean_x, mean_y);
#endif

  mean = mean_y;
  mean = (mean << 16) | mean_x;

  pos = 0;
  remain = count;

  while (remain) {
    if (remain > VAR_STEP)
      step = VAR_STEP;
    else
      step = remain;

    cal_variance_covariance_simd(&data_x[pos], &data_y[pos], mean, step, &var, &covar);
#if (DEBUG == 1)
    printf("var %lu, covar %lu\n", var, covar);
#endif
    pos += step;
    remain -= step;
    var_d += var;
    covar_d += covar;
  }

  *m = covar_d/var_d;
  *c = (double)(mean_y) - *m * (double)mean_x;
}



int main(int argc, char *argv[])
{
  uint16_t *data_x;
  uint16_t *data_y;
  uint64_t i;
  struct timeval tv1, tv2, tv;
  float m, c;

  if ((data_x = malloc(SIZE * sizeof(*data_x))) == NULL) {
    fprintf(stderr, "memory allocation error\n");
    goto exit;
  }

  if ((data_y = malloc(SIZE * sizeof(*data_y))) == NULL) {
    fprintf(stderr, "memory allocation error\n");
    goto exit;
  }

  printf("Filling random data...");
  fflush(stdout);

  srand(time(0));

  for (i=0; i< SIZE; i++) {
    data_x[i] = rand() & 0xffff;
    data_y[i] = GRADIENT * data_x[i] + OFFSET;
    // Add some noise
    if (data_y[i] > 512) {
      if (i & 0x01)
        data_y[i] += rand() & 0xff;
      else
        data_y[i] -= rand() & 0xff;
    }
  }

  printf("\n");
  fflush(stdout);


  gettimeofday(&tv1, NULL);
  find_linear_regression_c(data_x, data_y, SIZE, &m, &c);
  gettimeofday(&tv2, NULL);

  printf("m = %2.1f, c = %2.1f\n", m, c);
  timersub(&tv2, &tv1, &tv);
  printf("time %ld.%06ld\n", tv.tv_sec, tv.tv_usec);

  gettimeofday(&tv1, NULL);
  find_linear_regression_simd(data_x, data_y, SIZE, &m, &c);
  gettimeofday(&tv2, NULL);

  printf("m = %2.1f, c = %2.1f\n", m, c);
  timersub(&tv2, &tv1, &tv);
  printf("time %ld.%06ld\n", tv.tv_sec, tv.tv_usec);

exit:
  if (data_x)
    free(data_x);

  if (data_y)
    free(data_y);

  return 0;
}
