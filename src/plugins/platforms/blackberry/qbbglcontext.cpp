/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qbbglcontext.h"
#include "qbbrootwindow.h"
#include "qbbscreen.h"
#include "qbbwindow.h"

#include "private/qeglconvenience_p.h"

#include <QtCore/QDebug>
#include <QtGui/QOpenGLContext>

QT_BEGIN_NAMESPACE

EGLDisplay QBBGLContext::ms_eglDisplay = EGL_NO_DISPLAY;

static EGLenum checkEGLError(const char *msg)
{
    static const char *errmsg[] =
    {
        "EGL function succeeded",
        "EGL is not initialized, or could not be initialized, for the specified display",
        "EGL cannot access a requested resource",
        "EGL failed to allocate resources for the requested operation",
        "EGL fail to access an unrecognized attribute or attribute value was passed in an attribute list",
        "EGLConfig argument does not name a valid EGLConfig",
        "EGLContext argument does not name a valid EGLContext",
        "EGL current surface of the calling thread is no longer valid",
        "EGLDisplay argument does not name a valid EGLDisplay",
        "EGL arguments are inconsistent",
        "EGLNativePixmapType argument does not refer to a valid native pixmap",
        "EGLNativeWindowType argument does not refer to a valid native window",
        "EGL one or more argument values are invalid",
        "EGLSurface argument does not name a valid surface configured for rendering",
        "EGL power management event has occurred",
    };
    EGLenum error = eglGetError();
    fprintf(stderr, "%s: %s\n", msg, errmsg[error - EGL_SUCCESS]);
    return error;
}

QBBGLContext::QBBGLContext(QOpenGLContext *glContext)
    : QPlatformOpenGLContext(),
      m_glContext(glContext),
      m_eglSurface(EGL_NO_SURFACE)
{
#if defined(QBBGLCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    QSurfaceFormat format = m_glContext->format();

    // Set current rendering API
    EGLBoolean eglResult = eglBindAPI(EGL_OPENGL_ES_API);
    if (eglResult != EGL_TRUE) {
        qFatal("QBB: failed to set EGL API, err=%d", eglGetError());
    }

    // Get colour channel sizes from window format
    int alphaSize = format.alphaBufferSize();
    int redSize = format.redBufferSize();
    int greenSize = format.greenBufferSize();
    int blueSize = format.blueBufferSize();

    // Check if all channels are don't care
    if (alphaSize == -1 && redSize == -1 && greenSize == -1 && blueSize == -1) {
        // Set colour channels based on depth of window's screen
        QBBScreen *screen = static_cast<QBBScreen*>(QBBScreen::screens().first());
        int depth = screen->depth();
        if (depth == 32) {
            // SCREEN_FORMAT_RGBA8888
            alphaSize = 8;
            redSize = 8;
            greenSize = 8;
            blueSize = 8;
        } else {
            // SCREEN_FORMAT_RGB565
            alphaSize = 0;
            redSize = 5;
            greenSize = 6;
            blueSize = 5;
        }
    } else {
        // Choose best match based on supported pixel formats
        if (alphaSize <= 0 && redSize <= 5 && greenSize <= 6 && blueSize <= 5) {
            // SCREEN_FORMAT_RGB565
            alphaSize = 0;
            redSize = 5;
            greenSize = 6;
            blueSize = 5;
        } else {
            // SCREEN_FORMAT_RGBA8888
            alphaSize = 8;
            redSize = 8;
            greenSize = 8;
            blueSize = 8;
        }
    }

    // Update colour channel sizes in window format
    format.setAlphaBufferSize(alphaSize);
    format.setRedBufferSize(redSize);
    format.setGreenBufferSize(greenSize);
    format.setBlueBufferSize(blueSize);
    format.setSamples(2);

    // Select EGL config based on requested window format
    m_eglConfig = q_configFromGLFormat(ms_eglDisplay, format);
    if (m_eglConfig == 0) {
        qFatal("QBB: failed to find EGL config");
    }

    m_eglContext = eglCreateContext(ms_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, contextAttrs());
    if (m_eglContext == EGL_NO_CONTEXT) {
        checkEGLError("eglCreateContext");
        qFatal("QBB: failed to create EGL context, err=%d", eglGetError());
    }

    // Query/cache window format of selected EGL config
    m_windowFormat = q_glFormatFromConfig(ms_eglDisplay, m_eglConfig);
}

QBBGLContext::~QBBGLContext()
{
#if defined(QBBGLCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    // Cleanup EGL context if it exists
    if (m_eglContext != EGL_NO_CONTEXT) {
        eglDestroyContext(ms_eglDisplay, m_eglContext);
    }

    // Cleanup EGL surface if it exists
    destroySurface();
}

void QBBGLContext::initialize()
{
#if defined(QBBGLCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    // Initialize connection to EGL
    ms_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (ms_eglDisplay == EGL_NO_DISPLAY) {
        checkEGLError("eglGetDisplay");
        qFatal("QBB: failed to obtain EGL display");
    }

    EGLBoolean eglResult = eglInitialize(ms_eglDisplay, 0, 0);
    if (eglResult != EGL_TRUE) {
        checkEGLError("eglInitialize");
        qFatal("QBB: failed to initialize EGL display, err=%d", eglGetError());
    }
}

void QBBGLContext::shutdown()
{
#if defined(QBBGLCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    // Close connection to EGL
    eglTerminate(ms_eglDisplay);
}

bool QBBGLContext::makeCurrent(QPlatformSurface *surface)
{
#if defined(QBBGLCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    // Set current rendering API
    EGLBoolean eglResult = eglBindAPI(EGL_OPENGL_ES_API);
    if (eglResult != EGL_TRUE) {
        qFatal("QBB: failed to set EGL API, err=%d", eglGetError());
    }

    if (m_eglSurface == EGL_NO_SURFACE)
        createSurface(surface);

    eglResult = eglMakeCurrent(ms_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);
    if (eglResult != EGL_TRUE) {
        checkEGLError("eglMakeCurrent");
        qFatal("QBB: failed to set current EGL context, err=%d", eglGetError());
    }
    return (eglResult == EGL_TRUE);
}

void QBBGLContext::doneCurrent()
{
#if defined(QBBGLCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    // set current rendering API
    EGLBoolean eglResult = eglBindAPI(EGL_OPENGL_ES_API);
    if (eglResult != EGL_TRUE) {
        qFatal("QBB: failed to set EGL API, err=%d", eglGetError());
    }

    // clear curent EGL context and unbind EGL surface
    eglResult = eglMakeCurrent(ms_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (eglResult != EGL_TRUE) {
        qFatal("QBB: failed to clear current EGL context, err=%d", eglGetError());
    }
}

void QBBGLContext::swapBuffers(QPlatformSurface *surface)
{
    Q_UNUSED(surface);
#if defined(QBBGLCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    // Set current rendering API
    EGLBoolean eglResult = eglBindAPI(EGL_OPENGL_ES_API);
    if (eglResult != EGL_TRUE) {
        qFatal("QBB: failed to set EGL API, err=%d", eglGetError());
    }

    // Post EGL surface to window
    eglResult = eglSwapBuffers(ms_eglDisplay, m_eglSurface);
    if (eglResult != EGL_TRUE) {
        qFatal("QBB: failed to swap EGL buffers, err=%d", eglGetError());
    }
}

QFunctionPointer QBBGLContext::getProcAddress(const QByteArray &procName)
{
#if defined(QBBGLCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    // Set current rendering API
    EGLBoolean eglResult = eglBindAPI(EGL_OPENGL_ES_API);
    if (eglResult != EGL_TRUE) {
        qFatal("QBB: failed to set EGL API, err=%d", eglGetError());
    }

    // Lookup EGL extension function pointer
    return static_cast<QFunctionPointer>(eglGetProcAddress(procName.constData()));
}

EGLint *QBBGLContext::contextAttrs()
{
#if defined(QBBGLCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    // Choose EGL settings based on OpenGL version
#if defined(QT_OPENGL_ES_2)
    static EGLint attrs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    return attrs;
#else
    return 0;
#endif
}

bool QBBGLContext::isCurrent() const
{
#if defined(QBBGLCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    return (eglGetCurrentContext() == m_eglContext);
}

void QBBGLContext::createSurface(QPlatformSurface *surface)
{
#if defined(QBBGLCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    // Get a pointer to the corresponding platform window
    QBBWindow *platformWindow = dynamic_cast<QBBWindow*>(surface);
    if (!platformWindow) {
        qFatal("QBB: unable to create EGLSurface without a QBBWindow");
    }

    // If the platform window does not yet have any buffers, we create
    // a temporary set of buffers with a size of 1x1 pixels. This will
    // suffice until such time as the platform window has obtained
    // buffers of the proper size
    if (!platformWindow->hasBuffers()) {
        platformWindow->setPlatformOpenGLContext(this);
        m_surfaceSize = platformWindow->geometry().size();
        platformWindow->setBufferSize(m_surfaceSize);
    }

    // Obtain the native handle for our window
    screen_window_t handle = platformWindow->nativeHandle();

    const EGLint eglSurfaceAttrs[] =
    {
        EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
        EGL_NONE
    };

    // Create EGL surface
    m_eglSurface = eglCreateWindowSurface(ms_eglDisplay, m_eglConfig, (EGLNativeWindowType) handle, eglSurfaceAttrs);
    if (m_eglSurface == EGL_NO_SURFACE) {
        checkEGLError("eglCreateWindowSurface");
        qFatal("QBB: failed to create EGL surface, err=%d", eglGetError());
    }
}

void QBBGLContext::destroySurface()
{
#if defined(QBBGLCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    // Destroy EGL surface if it exists
    if (m_eglSurface != EGL_NO_SURFACE) {
        EGLBoolean eglResult = eglDestroySurface(ms_eglDisplay, m_eglSurface);
        if (eglResult != EGL_TRUE) {
            qFatal("QBB: failed to destroy EGL surface, err=%d", eglGetError());
        }
    }
}

QT_END_NAMESPACE
