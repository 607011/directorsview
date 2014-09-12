// Copyright (c) 2014 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.


#include "kernel.h"

const QVector2D Kernel::TexCoords[4] =
{
    QVector2D(1, 0),
    QVector2D(1, 1),
    QVector2D(0, 0),
    QVector2D(0, 1)
};
const QVector2D Kernel::TexCoords4FBO[4] =
{
    QVector2D(1, 1),
    QVector2D(1, 0),
    QVector2D(0, 1),
    QVector2D(0, 0)
};
const QVector2D Kernel::Vertices[4] =
{
    QVector2D( 1.0,  1.0),
    QVector2D( 1.0, -1.0),
    QVector2D(-1.0,  1.0),
    QVector2D(-1.0, -1.0)
};


Kernel::Kernel(void)
    : program(new QGLShaderProgram)
    , vertexShader(new QGLShader(QGLShader::Vertex))
    , fragmentShader(new QGLShader(QGLShader::Fragment))
{ /* ... */ }


Kernel::~Kernel()
{
    program->removeAllShaders();
    safeDelete(fragmentShader);
    safeDelete(vertexShader);
    safeDelete(program);
}


bool Kernel::setShaders(const QString &vs, const QString &fs) {
    if (vs.isEmpty() || fs.isEmpty())
        return false;
    bool ok = true;
    try {
        ok = vertexShader->compileSourceCode(vs);
        qDebug() << "VERTEX SHADER COMPILED:" << vertexShader->isCompiled();
        qDebug() << "VERTEX SHADER COMPILATION LOG:" << vertexShader->log();
        if (!ok)
            return false;
        ok = fragmentShader->compileSourceCode(fs);
        qDebug() << "FRAGMENT SHADER COMPILED:" << fragmentShader->isCompiled();
        qDebug() << "FRAGMENT SHADER COMPILATION LOG:" << fragmentShader->log();
        if (!ok)
            return false;
    }
    catch (...) {
        qFatal("memory allocation error");
        return false;
    }
    program->removeAllShaders();
    program->addShader(vertexShader);
    program->addShader(fragmentShader);
    program->link();
    program->bindAttributeLocation("aVertex", AVERTEX);
    program->bindAttributeLocation("aTexCoord", ATEXCOORD);
    program->bind();
    program->enableAttributeArray(AVERTEX);
    program->enableAttributeArray(ATEXCOORD);
    program->setAttributeArray(AVERTEX, Vertices);
    uLocResolution = program->uniformLocation("uResolution");
    uLocTexture = program->uniformLocation("uTexture");
    uLocGazePoint = program->uniformLocation("uGazePoint");
    uLocPeepholeRadius = program->uniformLocation("uPeepholeRadius");
    return ok;
}

bool Kernel::isFunctional(void) const {
    return program->isLinked();
}

