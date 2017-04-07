/*
 * Copyright (c) 2012 Carsten Munk <carsten.munk@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


#define _LARGEFILE64_SOURCE 1
/* EGL function pointers */
#define EGL_EGLEXT_PROTOTYPES
/* For RTLD_DEFAULT */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <syscall.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <dlfcn.h>
#include <stddef.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

static void *_libmali = NULL;

static EGLint  (*_eglGetError)(void) = NULL;

static EGLDisplay  (*_eglGetDisplay)(EGLNativeDisplayType display_id) = NULL;
static EGLBoolean  (*_eglInitialize)(EGLDisplay dpy, EGLint *major, EGLint *minor) = NULL;
static EGLBoolean  (*_eglTerminate)(EGLDisplay dpy) = NULL;

static const char *  (*_eglQueryString)(EGLDisplay dpy, EGLint name) = NULL;

static EGLBoolean  (*_eglGetConfigs)(EGLDisplay dpy, EGLConfig *configs,
		EGLint config_size, EGLint *num_config) = NULL;
static EGLBoolean  (*_eglChooseConfig)(EGLDisplay dpy, const EGLint *attrib_list,
		EGLConfig *configs, EGLint config_size,
		EGLint *num_config) = NULL;
static EGLBoolean  (*_eglGetConfigAttrib)(EGLDisplay dpy, EGLConfig config,
		EGLint attribute, EGLint *value) = NULL;

static EGLSurface  (*_eglCreateWindowSurface)(EGLDisplay dpy, EGLConfig config,
		EGLNativeWindowType win,
		const EGLint *attrib_list) = NULL;
static EGLSurface  (*_eglCreatePbufferSurface)(EGLDisplay dpy, EGLConfig config,
		const EGLint *attrib_list) = NULL;
static EGLSurface  (*_eglCreatePixmapSurface)(EGLDisplay dpy, EGLConfig config,
		EGLNativePixmapType pixmap,
		const EGLint *attrib_list) = NULL;
static EGLBoolean  (*_eglDestroySurface)(EGLDisplay dpy, EGLSurface surface) = NULL;
static EGLBoolean  (*_eglQuerySurface)(EGLDisplay dpy, EGLSurface surface,
		EGLint attribute, EGLint *value) = NULL;

static EGLBoolean  (*_eglBindAPI)(EGLenum api) = NULL;
static EGLenum  (*_eglQueryAPI)(void) = NULL;

static EGLBoolean  (*_eglWaitClient)(void) = NULL;

static EGLBoolean  (*_eglReleaseThread)(void) = NULL;

static EGLSurface  (*_eglCreatePbufferFromClientBuffer)(
		EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer,
		EGLConfig config, const EGLint *attrib_list) = NULL;

static EGLBoolean  (*_eglSurfaceAttrib)(EGLDisplay dpy, EGLSurface surface,
		EGLint attribute, EGLint value) = NULL;
static EGLBoolean  (*_eglBindTexImage)(EGLDisplay dpy, EGLSurface surface, EGLint buffer) = NULL;
static EGLBoolean  (*_eglReleaseTexImage)(EGLDisplay dpy, EGLSurface surface, EGLint buffer) = NULL;


static EGLBoolean  (*_eglSwapInterval)(EGLDisplay dpy, EGLint interval) = NULL;


static EGLContext  (*_eglCreateContext)(EGLDisplay dpy, EGLConfig config,
		EGLContext share_context,
		const EGLint *attrib_list) = NULL;
static EGLBoolean  (*_eglDestroyContext)(EGLDisplay dpy, EGLContext ctx) = NULL;
static EGLBoolean  (*_eglMakeCurrent)(EGLDisplay dpy, EGLSurface draw,
		EGLSurface read, EGLContext ctx) = NULL;

static EGLContext  (*_eglGetCurrentContext)(void) = NULL;
static EGLSurface  (*_eglGetCurrentSurface)(EGLint readdraw) = NULL;
static EGLDisplay  (*_eglGetCurrentDisplay)(void) = NULL;
static EGLBoolean  (*_eglQueryContext)(EGLDisplay dpy, EGLContext ctx,
		EGLint attribute, EGLint *value) = NULL;

static EGLBoolean  (*_eglWaitGL)(void) = NULL;
static EGLBoolean  (*_eglWaitNative)(EGLint engine) = NULL;
static EGLBoolean  (*_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface) = NULL;
static EGLBoolean  (*_eglCopyBuffers)(EGLDisplay dpy, EGLSurface surface,
		EGLNativePixmapType target) = NULL;


static EGLImageKHR (*_eglCreateImageKHR)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list) = NULL;
static EGLBoolean (*_eglDestroyImageKHR) (EGLDisplay dpy, EGLImageKHR image) = NULL;

static void (*_glEGLImageTargetTexture2DOES) (GLenum target, GLeglImageOES image) = NULL;

static __eglMustCastToProperFunctionPointerType (*_eglGetProcAddress)(const char *procname) = NULL;

static void _init_maliegl()
{
	_libmali = (void *) dlopen(getenv("LIBMALI") ? getenv("LIBMALI") : "libMali.so", RTLD_LAZY);
	if (!_libmali) {
		fprintf(stderr, "failed to Open %s\n", getenv("LIBMALI") ? getenv("LIBMALI") : "libMali.so");
		exit(1);
	}
	fprintf(stderr, "====%s=====\n", __func__);
}

#define EGL_DLSYM(fptr, sym) 											\
	do { 																\
		if (_libmali == NULL) {											\
			_init_maliegl();											\
		}																\
		if (*(fptr) == NULL) {											\
			*(fptr) = (void *) dlsym(_libmali, sym);					\
			if (!*(fptr)) {												\
				fprintf(stderr, "failed to find symbol %s\n", sym);		\
				exit(1);												\
			}															\
			fprintf(stderr, "====%s=%p====\n", __func__, *(fptr));		\
		}																\
	} while (0)

EGLint eglGetError(void)
{
	EGL_DLSYM(&_eglGetError, "eglGetError");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglGetError)();
}

EGLDisplay eglGetDisplay(EGLNativeDisplayType display_id)
{
	EGL_DLSYM(&_eglGetDisplay, "eglGetDisplay");
	fprintf(stderr, "====%s=%p====\n", __func__, *_eglGetDisplay);
	return (*_eglGetDisplay)(display_id);
}

EGLBoolean eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
	EGL_DLSYM(&_eglInitialize, "eglInitialize");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglInitialize)(dpy, major, minor);
}

EGLBoolean eglTerminate(EGLDisplay dpy)
{
	EGL_DLSYM(&_eglTerminate, "eglTerminate");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglTerminate)(dpy);
}

const char * eglQueryString(EGLDisplay dpy, EGLint name)
{
	EGL_DLSYM(&_eglQueryString, "eglQueryString");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglQueryString)(dpy, name);
}

EGLBoolean eglGetConfigs(EGLDisplay dpy, EGLConfig *configs,
		EGLint config_size, EGLint *num_config)
{
	EGL_DLSYM(&_eglGetConfigs, "eglGetConfigs");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglGetConfigs)(dpy, configs, config_size, num_config);
}

EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list,
		EGLConfig *configs, EGLint config_size,
		EGLint *num_config)
{
	EGL_DLSYM(&_eglChooseConfig, "eglChooseConfig");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglChooseConfig)(dpy, attrib_list,
			configs, config_size,
			num_config);
}

EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config,
		EGLint attribute, EGLint *value)
{
	EGL_DLSYM(&_eglGetConfigAttrib, "eglGetConfigAttrib");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglGetConfigAttrib)(dpy, config,
			attribute, value);
}

EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config,
		EGLNativeWindowType win,
		const EGLint *attrib_list)
{
	EGL_DLSYM(&_eglCreateWindowSurface, "eglCreateWindowSurface");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglCreateWindowSurface)(dpy, config, win, attrib_list);
}

EGLSurface eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config,
		const EGLint *attrib_list)
{
	EGL_DLSYM(&_eglCreatePbufferSurface, "eglCreatePbufferSurface");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglCreatePbufferSurface)(dpy, config, attrib_list);
}

EGLSurface eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config,
		EGLNativePixmapType pixmap,
		const EGLint *attrib_list)
{
	EGL_DLSYM(&_eglCreatePixmapSurface, "eglCreatePixmapSurface");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglCreatePixmapSurface)(dpy, config, pixmap, attrib_list);
}

EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
	EGL_DLSYM(&_eglDestroySurface, "eglDestroySurface");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglDestroySurface)(dpy, surface);
}

EGLBoolean eglQuerySurface(EGLDisplay dpy, EGLSurface surface,
		EGLint attribute, EGLint *value)
{
	EGL_DLSYM(&_eglQuerySurface, "eglQuerySurface");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglQuerySurface)(dpy, surface, attribute, value);
}

EGLBoolean eglBindAPI(EGLenum api)
{
	EGL_DLSYM(&_eglBindAPI, "eglBindAPI");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglBindAPI)(api);
}

EGLenum eglQueryAPI(void)
{
	EGL_DLSYM(&_eglQueryAPI, "eglQueryAPI");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglQueryAPI)();
}

EGLBoolean eglWaitClient(void)
{
	EGL_DLSYM(&_eglWaitClient, "eglWaitClient");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglWaitClient)();
}

EGLBoolean eglReleaseThread(void)
{
	EGL_DLSYM(&_eglReleaseThread, "eglReleaseThread");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglReleaseThread)();
}

EGLSurface eglCreatePbufferFromClientBuffer(
		EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer,
		EGLConfig config, const EGLint *attrib_list)
{
	EGL_DLSYM(&_eglCreatePbufferFromClientBuffer, "eglCreatePbufferFromClientBuffer");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglCreatePbufferFromClientBuffer)(dpy, buftype, buffer, config, attrib_list);
}

EGLBoolean eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface,
		EGLint attribute, EGLint value)
{
	EGL_DLSYM(&_eglSurfaceAttrib, "eglSurfaceAttrib");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglSurfaceAttrib)(dpy, surface, attribute, value);
}

EGLBoolean eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
	EGL_DLSYM(&_eglBindTexImage, "eglBindTexImage");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglBindTexImage)(dpy, surface, buffer);
}

EGLBoolean eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
	EGL_DLSYM(&_eglReleaseTexImage, "eglReleaseTexImage");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglReleaseTexImage)(dpy, surface, buffer);
}

EGLBoolean eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
	EGL_DLSYM(&_eglGetCurrentSurface, "eglGetCurrentSurface");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglSwapInterval)(dpy, interval);
}

EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config,
		EGLContext share_context,
		const EGLint *attrib_list)
{
	EGL_DLSYM(&_eglCreateContext, "eglCreateContext");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglCreateContext)(dpy, config, share_context, attrib_list);
}

EGLBoolean eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
	EGL_DLSYM(&_eglDestroyContext, "eglDestroyContext");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglDestroyContext)(dpy, ctx);
}

EGLBoolean eglMakeCurrent(EGLDisplay dpy, EGLSurface draw,
		EGLSurface read, EGLContext ctx)
{
	EGL_DLSYM(&_eglMakeCurrent, "eglMakeCurrent");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglMakeCurrent)(dpy, draw, read, ctx);
}

EGLContext eglGetCurrentContext(void)
{
	EGL_DLSYM(&_eglGetCurrentContext, "eglGetCurrentContext");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglGetCurrentContext)();
}

EGLSurface eglGetCurrentSurface(EGLint readdraw)
{
	EGL_DLSYM(&_eglGetCurrentSurface, "eglGetCurrentSurface");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglGetCurrentSurface)(readdraw);
}

EGLDisplay eglGetCurrentDisplay(void)
{
	EGL_DLSYM(&_eglGetCurrentDisplay, "eglGetCurrentDisplay");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglGetCurrentDisplay)();
}

EGLBoolean eglQueryContext(EGLDisplay dpy, EGLContext ctx,
		EGLint attribute, EGLint *value)
{
	EGL_DLSYM(&_eglQueryContext, "eglQueryContext");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglQueryContext)(dpy, ctx, attribute, value);
}

EGLBoolean eglWaitGL(void)
{
	EGL_DLSYM(&_eglWaitGL, "eglWaitGL");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglWaitGL)();
}

EGLBoolean eglWaitNative(EGLint engine)
{
	EGL_DLSYM(&_eglWaitNative, "eglWaitNative");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglWaitNative)(engine);
}

EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
	EGL_DLSYM(&_eglSwapBuffers, "eglSwapBuffers");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglSwapBuffers)(dpy, surface);
}

EGLBoolean eglCopyBuffers(EGLDisplay dpy, EGLSurface surface,
		EGLNativePixmapType target)
{
	EGL_DLSYM(&_eglCopyBuffers, "eglCopyBuffers");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglCopyBuffers)(dpy, surface, target);
}

__eglMustCastToProperFunctionPointerType eglGetProcAddress(const char *procname)
{
	EGL_DLSYM(&_eglGetProcAddress, "eglGetProcAddress");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglGetProcAddress)(procname);
}

EGLBoolean eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image)
{
	EGL_DLSYM(&_eglDestroyImageKHR, "eglDestroyImageKHR");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglDestroyImageKHR)(dpy, image);
}

/* sys calls */

#ifdef __NR_mmap2
#define	SYS_mmap2 __NR_mmap2
#define	MMAP2_PAGE_SHIFT 12
#else
#define	SYS_mmap2 SYS_mmap
#define	MMAP2_PAGE_SHIFT 0
#endif

#define PROMOTED_MODE_T	mode_t

#undef SYS_OPEN
#undef SYS_CLOSE
#undef SYS_IOCTL
#undef SYS_READ
#undef SYS_WRITE
#undef SYS_MMAP
#undef SYS_MUNMAP

#define SYS_OPEN(file, oflag, mode) \
	syscall(SYS_open, (const char *)(file), (int)(oflag), (mode_t)(mode))
#define SYS_CLOSE(fd) \
	syscall(SYS_close, (int)(fd))
#define SYS_DUP(fd) \
	syscall(SYS_dup, (int)(fd))
#define SYS_IOCTL(fd, cmd, arg) \
	syscall(SYS_ioctl, (int)(fd), (unsigned long)(cmd), (void *)(arg))
#define SYS_READ(fd, buf, len) \
	syscall(SYS_read, (int)(fd), (void *)(buf), (size_t)(len));
#define SYS_WRITE(fd, buf, len) \
	syscall(SYS_write, (int)(fd), (const void *)(buf), (size_t)(len));
#define SYS_MMAP(addr, len, prot, flags, fd, off) \
	syscall(SYS_mmap2, (void *)(addr), (size_t)(len), \
			(int)(prot), (int)(flags), (int)(fd), (__off_t)((off) >> MMAP2_PAGE_SHIFT))
#define SYS_MUNMAP(addr, len) \
	syscall(SYS_munmap, (void *)(addr), (size_t)(len))

/* Check that open/read/mmap is not a define */
#if defined open || defined read || defined mmap
#error open/read/mmap is a prepocessor macro !!
#endif

#define WRAP_PUBLIC __attribute__ ((visibility("default")))

WRAP_PUBLIC int open(const char *file, int oflag, ...)
{
	int fd;

	fprintf(stderr, "====%s=%s===\n", __func__, file);

	if (oflag & O_CREAT) {
		va_list ap;
		mode_t mode;

		va_start(ap, oflag);
		mode = va_arg(ap, PROMOTED_MODE_T);

		fd = SYS_OPEN(file, oflag, mode);

		va_end(ap);
	} else
		fd = SYS_OPEN(file, oflag, 0);

	return fd;
}

#ifdef linux
WRAP_PUBLIC int open64(const char *file, int oflag, ...)
{
	int fd;

	fprintf(stderr, "====%s====\n", __func__);
	if (oflag & O_CREAT) {
		va_list ap;
		mode_t mode;

		va_start(ap, oflag);
		mode = va_arg(ap, mode_t);

		fd = SYS_OPEN(file, oflag | O_LARGEFILE, mode);

		va_end(ap);
	} else
		fd = SYS_OPEN(file, oflag | O_LARGEFILE, 0);

	return fd;
}
#endif

WRAP_PUBLIC int close(int fd)
{
	fprintf(stderr, "====%s====\n", __func__);
	return SYS_CLOSE(fd);
}

WRAP_PUBLIC int dup(int fd)
{
	fprintf(stderr, "====%s====\n", __func__);
	return SYS_DUP(fd);
}

WRAP_PUBLIC int ioctl(int fd, unsigned long int request, ...)
{
	void *arg;
	va_list ap;
	fprintf(stderr, "====%s====\n", __func__);

	va_start(ap, request);
	arg = va_arg(ap, void *);
	va_end(ap);

	return SYS_IOCTL(fd, request, arg);
}

WRAP_PUBLIC ssize_t read(int fd, void *buffer, size_t n)
{
	fprintf(stderr, "====%s====\n", __func__);
	return SYS_READ(fd, buffer, n);
}

WRAP_PUBLIC void *mmap(void *start, size_t length, int prot, int flags, int fd,
		__off_t offset)
{
	fprintf(stderr, "====%s====\n", __func__);
	return (void *)SYS_MMAP(start, length, prot, flags, fd, offset);
}

#ifdef linux
WRAP_PUBLIC void *mmap64(void *start, size_t length, int prot, int flags, int fd,
		__off64_t offset)
{
	fprintf(stderr, "====%s====\n", __func__);
	return (void *)SYS_MMAP(start, length, prot, flags, fd, offset);
}
#endif

WRAP_PUBLIC int munmap(void *start, size_t length)
{
	fprintf(stderr, "====%s====\n", __func__);
	return SYS_MUNMAP(start, length);
}
