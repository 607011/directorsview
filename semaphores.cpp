// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include "semaphores.h"

QSemaphore gFramesProduced(MAX_FRAMES_IN_QUEUE);
QSemaphore gFramesRendered(MAX_FRAMES_IN_QUEUE);
