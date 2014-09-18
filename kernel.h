// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __KERNEL_H_
#define __KERNEL_H_

#include <QVector>
#include <QGLShaderProgram>
#include <QGLShader>
#include <QVector2D>

#include "util.h"

class Kernel {
public:
    explicit Kernel(void);
    ~Kernel();
    bool setShaders(const QString &vsFile, const QString &fsFile);
    bool isFunctional(void) const;

public: // members
    QGLShaderProgram *program;
    QGLShader *fragmentShader;
    QGLShader *vertexShader;
    int uLocResolution;
    int uLocGazePoint;
    int uLocTexture;
    int uLocPeepholeRadius;

    enum { AVERTEX, ATEXCOORD };

    static const QVector2D TexCoords[4];
    static const QVector2D TexCoords4FBO[4];
    static const QVector2D Vertices[4];
};

typedef QVector<Kernel *> KernelList;


#endif // __KERNEL_H_
