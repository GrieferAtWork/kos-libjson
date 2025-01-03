/* Copyright (c) 2019-2025 Griefer@Work                                       *
 *                                                                            *
 * This software is provided 'as-is', without any express or implied          *
 * warranty. In no event will the authors be held liable for any damages      *
 * arising from the use of this software.                                     *
 *                                                                            *
 * Permission is granted to anyone to use this software for any purpose,      *
 * including commercial applications, and to alter it and redistribute it     *
 * freely, subject to the following restrictions:                             *
 *                                                                            *
 * 1. The origin of this software must not be misrepresented; you must not    *
 *    claim that you wrote the original software. If you use this software    *
 *    in a product, an acknowledgement (see the following) in the product     *
 *    documentation is required:                                              *
 *    Portions Copyright (c) 2019-2025 Griefer@Work                           *
 * 2. Altered source versions must be plainly marked as such, and must not be *
 *    misrepresented as being the original software.                          *
 * 3. This notice may not be removed or altered from any source distribution. *
 */
#ifndef GUARD_LIBJSON_API_H
#define GUARD_LIBJSON_API_H 1

#ifndef LIBJSON_NO_SYSTEM_INCLUDES
#include <hybrid/compiler.h>
#include <kos/config/config.h> /* Pull in config-specific macro overrides */
#include <libjson/api.h>
#endif /* !LIBJSON_NO_SYSTEM_INCLUDES */

#define CC LIBJSON_CC

/*[[[config CONFIG_LIBJSON_SUPPORTS_C_COMMENT = true]]]*/
#ifdef CONFIG_NO_LIBJSON_SUPPORTS_C_COMMENT
#undef CONFIG_LIBJSON_SUPPORTS_C_COMMENT
#elif !defined(CONFIG_LIBJSON_SUPPORTS_C_COMMENT)
#define CONFIG_LIBJSON_SUPPORTS_C_COMMENT
#elif (-CONFIG_LIBJSON_SUPPORTS_C_COMMENT - 1) == -1
#undef CONFIG_LIBJSON_SUPPORTS_C_COMMENT
#define CONFIG_NO_LIBJSON_SUPPORTS_C_COMMENT
#endif /* ... */
/*[[[end]]]*/

#endif /* !GUARD_LIBJSON_API_H */
