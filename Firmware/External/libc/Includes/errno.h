#ifndef ERRNO_H
#define ERRNO_H

// define errno constants; these mirror BSD-type systems

#define EPERM		1	/* Operation not permitted */
#define ENOENT		2	/* No such file or directory */
#define EIO		5	/* Input/output error */
#define ENXIO		6	/* Device not configured */
#define ENOMEM		12	/* Cannot allocate memory */
#define EFAULT		14	/* Bad address */
#define EBUSY		16	/* Device busy */
#define EEXIST		17	/* File exists */
#define ENODEV		19	/* Operation not supported by device */
#define EINVAL		22	/* Invalid argument */

#define EDOM		33	/* Numerical argument out of domain */
#define ERANGE		34	/* Result too large */


#endif
