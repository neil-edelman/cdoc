/** URL-encode roughly uses [PHP](https://www.php.net/license/).

 @title UrlEncode
 @license
 --------------------------------------------------------------------
 The PHP License, version 3.01
 Copyright (c) 1999 - 2019 The PHP Group. All rights reserved.
 --------------------------------------------------------------------
 
 @license
 Redistribution and use in source and binary forms, with or without
 modification, is permitted provided that the following conditions
 are met:
 
 @license
 1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 
 @license
 2. Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in
 the documentation and/or other materials provided with the
 distribution.
 
 @license
 3. The name "PHP" must not be used to endorse or promote products
 derived from this software without prior written permission. For
 written permission, please contact group@php.net.
 
 @license
 4. Products derived from this software may not be called "PHP", nor
 may "PHP" appear in their name, without prior written permission
 from group@php.net.  You may indicate that your software works in
 conjunction with PHP by saying "Foo for PHP" instead of calling
 it "PHP Foo" or "phpfoo"
 
 @license
 5. The PHP Group may publish revised and/or new versions of the
 license from time to time. Each version will be given a
 distinguishing version number.
 Once covered code has been published under a particular version
 of the license, you may always continue to use it under the terms
 of that version. You may also choose to use such covered code
 under the terms of any subsequent version of the license
 published by the PHP Group. No one other than the PHP Group has
 the right to modify the terms applicable to covered code created
 under this License.
 
 @license
 6. Redistributions of any form whatsoever must retain the following
 acknowledgment:
 "This product includes PHP software, freely available from
 <http://www.php.net/software/>".
 
 @license
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
 
 @license
 --------------------------------------------------------------------
 
 @license
 This software consists of voluntary contributions made by many
 individuals on behalf of the PHP Group.
 
 @license
 The PHP Group can be contacted via Email at group@php.net.
 
 @license
 For more information on the PHP Group and the PHP project,
 please see <http://www.php.net>.
 
 @license
 PHP includes the Zend Engine, freely available at
 <http://www.zend.com>.

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

/** URL encode the substring `s` with `length` to a static string of fixed
 maximum length.
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
