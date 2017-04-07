# Experimental work on an GBM wrapper for Fbdev libMali

The concept is to provide a standard GBM backend and use a similar technique as libhybris to fake a physical "fbdev" device with a runtime allocated DRM FbDev.

To achieve this, all the posix calls from libMali must be intercepted to provide a fake fbdev kernel interface to the libDRM and libGBM configured surface.

egl calls must also be intercepted to convert arguments from a GBM Window interface to a FBdev window interface.
