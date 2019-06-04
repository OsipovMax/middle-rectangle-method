#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <omp.h>  // time measurement

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
const double REAL_VALUE = 0.62053660344676220361;
double globalResult = 0.0;
int mainThreadsCount = 0;  // number of main threads (mod 1)

struct ThreadInfo {
  pthread_t threadId;
  int threadNum;
  int threadsCount;
  int intervalsCount;
  int statusFlag;  // identification of the main threads
};

double function(double x) { return sin(x * x); }

double absValue(double x) {
  if (x < 0.0) {
    return -x;
  }
  return x;
}

// integration function with additional threads
void *calculateIntegral1(void *argv) {
  int i = 0, j = 0, k = 0;
  double localResult = 0.0;
  double h = 0.0;
  double a = -1.0;
  double b = 1.0;
  int intervalPerProc = 0;
  int lBorder = 0, rBorder = 0;
  struct ThreadInfo *thrInfo = (struct ThreadInfo *)argv;
  struct ThreadInfo *additThrInfo;

  if (thrInfo->statusFlag == 1) {
    thrInfo->threadNum = thrInfo->threadNum * 4;
    additThrInfo = malloc(sizeof(*additThrInfo) * 3);
    for (j = 0; j < 3; j++) {
      additThrInfo[j].threadNum = j + thrInfo->threadNum + 1;
      additThrInfo[j].threadsCount = mainThreadsCount * 4;
      additThrInfo[j].intervalsCount = thrInfo->intervalsCount;
      additThrInfo[j].statusFlag = 0;
      pthread_create(&additThrInfo[j].threadId, NULL, calculateIntegral1,
                     &additThrInfo[j]);
    }
  }
  thrInfo->threadsCount = mainThreadsCount * 4;
  h = (b - a) / thrInfo->intervalsCount;
  intervalPerProc = thrInfo->intervalsCount / thrInfo->threadsCount;
  lBorder = thrInfo->threadNum * intervalPerProc;
  if ((thrInfo->threadNum == thrInfo->threadsCount - 1) &&
      thrInfo->intervalsCount % thrInfo->threadsCount != 0) {
    rBorder = thrInfo->intervalsCount;
  } else {
    rBorder = (thrInfo->threadNum + 1) * intervalPerProc;
  }
  for (i = lBorder; i < rBorder; i++) {
    localResult += function(a + h * (i + 0.5));
  }
  localResult *= h;
  pthread_mutex_lock(&lock);
  globalResult += localResult;
  pthread_mutex_unlock(&lock);
  if (thrInfo->statusFlag = 1) {
    for (k = 0; k < 3; k++) {
      pthread_join(additThrInfo[k].threadId, NULL);
    }
  }
  return NULL;
}

// standard integration function (no additional threads)
void *calculateIntegral(void *argv) {
  int i = 0, j = 0, extraThreads = 0;
  double localResult = 0.0;
  double h = 0.0;
  double a = -1.0;
  double b = 1.0;
  int intervalPerProc = 0;
  int lBorder = 0, rBorder = 0;
  struct ThreadInfo *thrInfo = (struct ThreadInfo *)argv;

  if (thrInfo->intervalsCount < thrInfo->threadsCount) {
    thrInfo->threadsCount = thrInfo->intervalsCount;
    if (thrInfo->threadNum >= thrInfo->intervalsCount) {
      pthread_mutex_lock(&lock);
      globalResult += 0.0;
      pthread_mutex_unlock(&lock);
      extraThreads++;
      return NULL;
    }
  }
  h = (b - a) / thrInfo->intervalsCount;
  intervalPerProc = thrInfo->intervalsCount / thrInfo->threadsCount;
  lBorder = thrInfo->threadNum * intervalPerProc;
  if ((thrInfo->threadNum == thrInfo->threadsCount - 1) &&
      thrInfo->intervalsCount % thrInfo->threadsCount != 0) {
    rBorder = thrInfo->intervalsCount;
  } else {
    rBorder = (thrInfo->threadNum + 1) * intervalPerProc;
  }
  for (i = lBorder; i < rBorder; i++) {
    localResult += function(a + h * (i + 0.5));
  }
  localResult *= h;
  pthread_mutex_lock(&lock);
  globalResult += localResult;
  pthread_mutex_unlock(&lock);
  return NULL;
}

int main(int argc, char **argv) {
  int generalThreadsCount = 0;
  int generalIntervalsCount = 0;
  int i, j, k;
  double resultValue = 0.0;
  double beginTime, endTime;
  double resultTime;
  int mod = 0;

  if (argc != 4) {
    printf("Неверное число параметров! \n");
    exit(1);
  } else {
    generalIntervalsCount = atoi(argv[1]);
    generalThreadsCount = atoi(argv[2]);
    mod = atoi(argv[3]);
  }
  mainThreadsCount = generalThreadsCount;
  struct ThreadInfo *threadInfo =
      malloc(sizeof(*threadInfo) * generalThreadsCount);
  if (threadInfo == NULL) {
    printf("Недостаточно памяти! \n");
    exit(1);
  }
  beginTime = omp_get_wtime();
  for (i = 0; i < generalThreadsCount; i++) {
    threadInfo[i].threadNum = i;
    threadInfo[i].threadsCount = generalThreadsCount;
    threadInfo[i].intervalsCount = generalIntervalsCount;
    threadInfo[i].statusFlag = 1;
    // program operation mode (with or without additional threads)
    if (mod == 0) {
      pthread_create(&threadInfo[i].threadId, NULL, calculateIntegral,
                     &threadInfo[i]);
    } else {
      pthread_create(&threadInfo[i].threadId, NULL, calculateIntegral1,
                     &threadInfo[i]);
    }
  }
  for (j = 0; j < generalThreadsCount; j++) {
    pthread_join(threadInfo[j].threadId, NULL);
  }
  endTime = omp_get_wtime();
  resultTime = endTime - beginTime;

  printf("Полученное значение: интеграл sin(x * x) от %f до %f = %.15lf\n",
         -1.0, 1.0, globalResult);
  printf("Точное значение: интеграл sin(x * x) от %f до %f = %.15lf\n", -1.0,
         1.0, REAL_VALUE);
  printf("Абсолютная погрешность: %.15lf\n",
         absValue(REAL_VALUE - globalResult));
  printf("Относительная погрешность: %.15lf%\n",
         100 * absValue(REAL_VALUE - globalResult) / absValue(REAL_VALUE));
  printf("Время работы программы %.15f\n", resultTime);

  free(threadInfo);
  return 0;
}
