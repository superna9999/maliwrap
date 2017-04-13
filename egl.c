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
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <dlfcn.h>
#include <stddef.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <linux/fb.h>

static void *_libmali = NULL;

static EGLint  (*_eglGetError)(void) = NULL;

static EGLDisplay  (*_eglGetDisplay)(EGLNativeDisplayType display_id) = NULL;
static EGLBoolean  (*_eglInitialize)(EGLDisplay dpy, EGLint *major, EGLint *minor) = NULL;
static EGLBoolean  (*_eglTerminate)(EGLDisplay dpy) = NULL;

static const char *  (*_eglQueryString)(EGLDisplay dpy, EGLint name) = NULL;
static void (*_glGetIntegerv)(GLenum pname, GLint *params) = NULL;
void (*_glViewport)(GLint x, GLint y, GLsizei width, GLsizei height) = NULL;
void (*_glMatrixMode)(GLenum mode) = NULL;

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

static const GLubyte * (*_glGetString)(GLenum name);

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

const GLubyte* glGetString(GLenum name)
{
	EGL_DLSYM(&_glGetString, "glGetString");
	fprintf(stderr, "====%s(%d)====\n", __func__);
	return (void *)(*_glGetString)(name);
}

void glGetIntegerv (GLenum pname, GLint *params)
{
	EGL_DLSYM(&_glGetIntegerv, "glGetIntegerv");
	fprintf(stderr, "====%s====\n", __func__);
	(*_glGetIntegerv)(pname, params);
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	EGL_DLSYM(&_glViewport, "glViewport");
	fprintf(stderr, "====%s====\n", __func__);
	(*_glViewport)(x, y, width, height);
}

EGLint eglGetError(void)
{
	EGL_DLSYM(&_eglGetError, "eglGetError");
	fprintf(stderr, "====%s====\n", __func__);
	return (*_eglGetError)();
}

EGLDisplay eglGetDisplay(EGLNativeDisplayType display_id)
{
	EGLDisplay disp;
	EGL_DLSYM(&_eglGetDisplay, "eglGetDisplay");
	fprintf(stderr, "====%s=%p=(%d)==\n", __func__, *_eglGetDisplay, display_id);
	disp = (*_eglGetDisplay)(display_id);
	fprintf(stderr, "=>%dp\n", disp);
	return disp;
}

EGLBoolean eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
	EGL_DLSYM(&_eglInitialize, "eglInitialize");
	fprintf(stderr, "====%s=(%p)===\n", __func__, dpy);
	return (*_eglInitialize)(dpy, major, minor);
}

EGLBoolean eglTerminate(EGLDisplay dpy)
{
	EGL_DLSYM(&_eglTerminate, "eglTerminate");
	fprintf(stderr, "====%s=(%p)===\n", __func__, dpy);
	return (*_eglTerminate)(dpy);
}

const char * eglQueryString(EGLDisplay dpy, EGLint name)
{
	EGL_DLSYM(&_eglQueryString, "eglQueryString");
	fprintf(stderr, "====%s=(%p, %s)===\n", __func__, dpy, name);
	return (*_eglQueryString)(dpy, name);
}

EGLBoolean eglGetConfigs(EGLDisplay dpy, EGLConfig *configs,
		EGLint config_size, EGLint *num_config)
{
	EGL_DLSYM(&_eglGetConfigs, "eglGetConfigs");
	fprintf(stderr, "====%s=(%p)==\n", __func__, dpy);
	return (*_eglGetConfigs)(dpy, configs, config_size, num_config);
}

EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list,
		EGLConfig *configs, EGLint config_size,
		EGLint *num_config)
{
	EGL_DLSYM(&_eglChooseConfig, "eglChooseConfig");
	fprintf(stderr, "====%s=(%p)===\n", __func__, dpy);
	return (*_eglChooseConfig)(dpy, attrib_list,
			configs, config_size,
			num_config);
}

EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config,
		EGLint attribute, EGLint *value)
{
	EGL_DLSYM(&_eglGetConfigAttrib, "eglGetConfigAttrib");
	fprintf(stderr, "====%s=(%p)===\n", __func__, dpy);
	return (*_eglGetConfigAttrib)(dpy, config,
			attribute, value);
}

EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config,
		EGLNativeWindowType win,
		const EGLint *attrib_list)
{
	EGLSurface srf;
	struct fbdev_window *native_display = win;
	EGL_DLSYM(&_eglCreateWindowSurface, "eglCreateWindowSurface");
	fprintf(stderr, "====%s=(%p, %d, %d, %d]=\n", __func__, dpy, config, native_display->width, native_display->height);
	srf = (*_eglCreateWindowSurface)(dpy, config, win, attrib_list);
	fprintf(stderr, "=>%p\n", srf);
	return srf;
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
	fprintf(stderr, "====%s=(%p, %p)==\n", __func__, dpy, surface);
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
	EGL_DLSYM(&_eglSwapInterval, "eglSwapInterval");
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
	fprintf(stderr, "====%s=(%p, %p)===\n", __func__, dpy, surface);
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
	__eglMustCastToProperFunctionPointerType ret = NULL;
	
	_init_maliegl();											\

	ret = (void *) dlsym(_libmali, procname);					\
	if (!ret) {												\
		fprintf(stderr, "failed to find symbol %s\n", procname);		\
		EGL_DLSYM(&_eglGetProcAddress, "eglGetProcAddress");
		fprintf(stderr, "====%s====\n", __func__);
		return (*_eglGetProcAddress)(procname);
	}
	else
		return ret;
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

#ifdef SYS_openat
#define SYS_OPEN(file, oflag, mode) \
	syscall(SYS_openat, AT_FDCWD, (const char *)(file), (int)(oflag), (mode_t)(mode))
#else
#define SYS_OPEN(file, oflag, mode) \
	syscall(SYS_open, (const char *)(file), (int)(oflag), (mode_t)(mode))
#endif
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
	syscall(SYS_mmap2, (long)(addr), (size_t)(len), \
			(int)(prot), (int)(flags), (int)(fd), (__off_t)((off) >> MMAP2_PAGE_SHIFT))
#define SYS_MUNMAP(addr, len) \
	syscall(SYS_munmap, (void *)(addr), (size_t)(len))

/* Check that open/read/mmap is not a define */
#if defined open || defined read || defined mmap
#error open/read/mmap is a prepocessor macro !!
#endif

#define WRAP_PUBLIC __attribute__ ((visibility("default")))

static fbdev_fd = -1;

WRAP_PUBLIC int open(const char *file, int oflag, ...)
{
	int fd;

	if (oflag & O_CREAT) {
		va_list ap;
		mode_t mode;

		va_start(ap, oflag);
		mode = va_arg(ap, PROMOTED_MODE_T);

		fd = SYS_OPEN(file, oflag, mode);

		va_end(ap);
	} else
		fd = SYS_OPEN(file, oflag, 0);

	fprintf(stderr, "====%s=%s=%d===\n", __func__, file, fd);

	if (!strcmp(file, "/dev/fb0"))
		fbdev_fd = fd;

	return fd;
}

#ifdef linux
WRAP_PUBLIC int open64(const char *file, int oflag, ...)
{
	int fd;

	if (oflag & O_CREAT) {
		va_list ap;
		mode_t mode;

		va_start(ap, oflag);
		mode = va_arg(ap, mode_t);

		fd = SYS_OPEN(file, oflag | O_LARGEFILE, mode);

		va_end(ap);
	} else
		fd = SYS_OPEN(file, oflag | O_LARGEFILE, 0);

	fprintf(stderr, "====%s=%s=%d===\n", __func__, file, fd);

	return fd;
}
#endif

WRAP_PUBLIC int close(int fd)
{
	fprintf(stderr, "====%s=%d===\n", __func__, fd);
	if (fd == fbdev_fd)
		fbdev_fd = -1;
	return SYS_CLOSE(fd);
}

WRAP_PUBLIC int dup(int fd)
{
	if (fd == fbdev_fd)
		fprintf(stderr, "====%s=%d===\n", __func__, fd);
	return SYS_DUP(fd);
}

static void print_var_screeninfo(void *p)
{
	struct fb_var_screeninfo *info = p;

	fprintf(stderr, " - xres = %d\n", info->xres);
	fprintf(stderr, " - yres = %d\n", info->yres);
	fprintf(stderr, " - xres_virtual = %d\n", info->xres_virtual);
	fprintf(stderr, " - yres_virtual = %d\n", info->yres_virtual);
	fprintf(stderr, " - xoffset = %d\n", info->xoffset);
	fprintf(stderr, " - yoffset = %d\n", info->yoffset);
	fprintf(stderr, " - bits_per_pixel = %d\n", info->bits_per_pixel);
	fprintf(stderr, " - grayscale = %d\n", info->grayscale);
	fprintf(stderr, " - red.length = %d\n", info->red.length);
	fprintf(stderr, " - green.length = %d\n", info->green.length);
	fprintf(stderr, " - blue.length = %d\n", info->blue.length);
	fprintf(stderr, " - transp.length = %d\n", info->transp.length);
	fprintf(stderr, " - nostd = %d\n", info->nonstd);
}

static void print_fix_screeninfo(void *p)
{
	struct fb_fix_screeninfo *info = p;

	fprintf(stderr, " - id = %s\n", info->id);
	fprintf(stderr, " - smem_start = %x\n", info->smem_start);
	fprintf(stderr, " - smem_len = %d\n", info->smem_len);
	fprintf(stderr, " - type = %d\n", info->type);
	fprintf(stderr, " - type_aux = %d\n", info->type_aux);
	fprintf(stderr, " - visual = %d\n", info->visual);
	fprintf(stderr, " - xpanstep = %d\n", info->xpanstep);
	fprintf(stderr, " - ypanstep = %d\n", info->ypanstep);
	fprintf(stderr, " - ywrapstep = %d\n", info->ywrapstep);
	fprintf(stderr, " - line_length = %d\n", info->line_length);
	fprintf(stderr, " - mmio_start = %x\n", info->mmio_start);
	fprintf(stderr, " - mmio_len = %d\n", info->mmio_len);
	fprintf(stderr, " - accel = %d\n", info->accel);
	fprintf(stderr, " - capabilities = %d\n", info->capabilities);
}

WRAP_PUBLIC int ioctl(int fd, unsigned long int request, ...)
{
	int ret;
	void *arg;
	va_list ap;

	va_start(ap, request);
	arg = va_arg(ap, void *);
	va_end(ap);

	if (fd == fbdev_fd) {
		fprintf(stderr, "====%s=%d=%x==(%d)\n", __func__, fd, request, getpid());
		
		switch(request) {
		case FBIOPUT_VSCREENINFO:
			fprintf(stderr, "==> FBIOPUT_VSCREENINFO\n");
			print_var_screeninfo(arg); 
			break;
		case FBIOPAN_DISPLAY:
			fprintf(stderr, "==> FBIOPAN_DISPLAY\n");
			print_var_screeninfo(arg); 
			break;
		case FBIO_WAITFORVSYNC:
			fprintf(stderr, "==> FBIO_WAITFORVSYNC\n");
			break;
		case FBIOGET_VSCREENINFO:
		case FBIOGET_FSCREENINFO:
			break;
		default:
			return 0;
		}
		
	}

	ret = SYS_IOCTL(fd, request, arg);

	if (fd == fbdev_fd) {
		switch(request) {
		case FBIOGET_VSCREENINFO:
			fprintf(stderr, "==> FBIOGET_VSCREENINFO\n");
			print_var_screeninfo(arg); 
			break;
		case FBIOGET_FSCREENINFO:
			fprintf(stderr, "==> FBIOGET_FSCREENINFO\n");
			print_fix_screeninfo(arg);
			break;
		}
	}

	return ret;
}

WRAP_PUBLIC ssize_t read(int fd, void *buffer, size_t n)
{
	if (fd == fbdev_fd)
		fprintf(stderr, "====%s=%d===\n", __func__, fd);
	return SYS_READ(fd, buffer, n);
}

WRAP_PUBLIC void *mmap(void *start, size_t length, int prot, int flags, int fd,
		__off_t offset)
{
	void *ret;	
	if (fd == fbdev_fd)
		fprintf(stderr, "====%s(%p, %lu, %d, %d, %d, %lu)==\n", __func__, start, length, prot, flags, fd, offset);
	ret = (void *)SYS_MMAP(start, length, prot, flags, fd, offset);
	if (fd == fbdev_fd)
		fprintf(stderr, "====%s=%p=====\n", __func__, ret);
	return ret;
}

#ifdef linux
WRAP_PUBLIC void *mmap64(void *start, size_t length, int prot, int flags, int fd,
		__off64_t offset)
{
	if (fd == fbdev_fd)
		fprintf(stderr, "====%s=%d=%x==\n", __func__, fd, offset);
	return (void *)SYS_MMAP(start, length, prot, flags, fd, offset);
}
#endif

WRAP_PUBLIC int munmap(void *start, size_t length)
{
	fprintf(stderr, "====%s=%p===\n", __func__, start);
	return SYS_MUNMAP(start, length);
}
