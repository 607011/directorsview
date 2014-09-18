// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __SEMAPHORES_H_
#define __SEMAPHORES_H_

#include <QSemaphore>

static const int MAX_FRAMES_IN_QUEUE = 64;

extern QSemaphore gFramesProduced;
extern QSemaphore gFramesRendered;

#endif // __SEMAPHORES_H_
