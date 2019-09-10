/** URL-encode a fixed length static string. Uses PHP.

 --------------------------------------------------------------------
 The PHP License, version 2.02
 Copyright (c) 1999 - 2002 The PHP Group. All rights reserved.
 --------------------------------------------------------------------
 
 Redistribution and use in source and binary forms, with or without
 modification, is permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above
 copyright notice, this list of conditions and the following
 disclaimer in the documentation and/or other materials provided
 with the distribution.
 
 3. The name "PHP" must not be used to endorse or promote products
 derived from this software without prior permission from the
 PHP Group.  This does not apply to add-on libraries or tools
 that work in conjunction with PHP.  In such a case the PHP
 name may be used to indicate that the product supports PHP.
 
 4. The PHP Group may publish revised and/or new versions of the
 license from time to time. Each version will be given a
 distinguishing version number.
 Once covered code has been published under a particular version
 of the license, you may always continue to use it under the
 terms of that version. You may also choose to use such covered
 code under the terms of any subsequent version of the license
 published by the PHP Group. No one other than the PHP Group has
 the right to modify the terms applicable to covered code created
 under this License.
 
 5. Redistributions of any form whatsoever must retain the following
 acknowledgment:
 "This product includes PHP, freely available from
 http://www.php.net/".
 
 6. The software incorporates the Zend Engine, a product of Zend
 Technologies, Ltd. ("Zend"). The Zend Engine is licensed to the
 PHP Association (pursuant to a grant from Zend that can be
 found at http://www.php.net/license/ZendGrant/) for
 distribution to you under this license agreement, only as a
 part of PHP.  In the event that you separate the Zend Engine
 (or any portion thereof) from the rest of the software, or
 modify the Zend Engine, or any portion thereof, your use of the
 separated or modified Zend Engine software shall not be governed
 by this license, and instead shall be governed by the license
 set forth at http://www.zend.com/license/ZendLicense/.
 
 
 
 THIS SOFTWARE IS PROVIDED BY THE PHP DEVELOPMENT TEAM ``AS IS'' AND
 ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE PHP
 DEVELOPMENT TEAM OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 OF THE POSSIBILITY OF SUCH DAMAGE.
 
 --------------------------------------------------------------------
 
 This software consists of voluntary contributions made by many
 individuals on behalf of the PHP Group.
 
 The PHP Group can be contacted via Email at group@php.net.
 
 For more information on the PHP Group and the PHP project,
 please see <http://www.php.net>.
 */

#include <ctype.h>  /* isalnum */
#include <string.h> /* strchr */
#include <errno.h>  /* errno ERANGE */
#include "UrlEncode.h"

/* rfc1738:
 
 ...The characters ";",
 "/", "?", ":", "@", "=" and "&" are the characters which may be
 reserved for special meaning within a scheme...
 
 ...Thus, only alphanumerics, the special characters "$-_.+!*'(),", and
 reserved characters used for their reserved purposes may be used
 unencoded within a URL...
 
 For added safety, we only leave -_. unencoded.
 */

static char hexchars[] = "0123456789ABCDEF";

/** URL encode `s` to a static string of fixed maximum length.
 @throws[ERANGE] The string could not be encoded in this length.
 @return A static string or null. */
const char *UrlEncode(char const *const s, size_t length) {
	static char encoded[64];
	unsigned char c;
	char *to = encoded;
	char const *from = s, *const end = from + length;

	while(from < end) {
		c = *from++;
		/* `php_raw_url_encode` encodes space as "%20", why? Google gets it. */
		if (c == ' ') {
			*to++ = '+';
		} else if (!isalnum(c) && strchr("_-.", c) == 0) {
			/* Allow only alphanumeric chars and '_', '-', '.'; escape the
			 rest */
			to[0] = '%';
			to[1] = hexchars[(unsigned char)c >> 4];
			to[2] = hexchars[(unsigned char)c & 15];
			to += 3;
		} else {
			*to++ = c;
		}
		if((size_t)(to - encoded) > sizeof encoded - 4)
			{ *to = '\0'; errno = ERANGE; return 0; }
	}
	*to = '\0';
	return encoded;
}
