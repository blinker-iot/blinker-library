/*
 * Copyright (c) 2013 Adam Rudd.
 * See LICENSE for more information
 */
#ifndef _BASE64_H
#define _BASE64_H

/* b64_alphabet:
 * 		Description: Base64 alphabet table, a mapping between integers
 * 					 and base64 digits
 * 		Notes: This is an extern here but is defined in Base64.c
 */
extern const char b64_alphabet[];

/* base64_encode:
 * 		Description:
 * 			Encode a string of characters as base64
 * 		Parameters:
 * 			output: the output buffer for the encoding, stores the encoded string
 * 			input: the input buffer for the encoding, stores the binary to be encoded
 * 			inputLen: the length of the input buffer, in bytes
 * 		Return value:
 * 			Returns the length of the encoded string
 * 		Requirements:
 * 			1. output must not be null or empty
 * 			2. input must not be null
 * 			3. inputLen must be greater than or equal to 0
 */
int base64_encode(char *output, char *input, int inputLen);

/* base64_decode:
 * 		Description:
 * 			Decode a base64 encoded string into bytes
 * 		Parameters:
 * 			output: the output buffer for the decoding,
 * 					stores the decoded binary
 * 			input: the input buffer for the decoding,
 * 				   stores the base64 string to be decoded
 * 			inputLen: the length of the input buffer, in bytes
 * 		Return value:
 * 			Returns the length of the decoded string
 * 		Requirements:
 * 			1. output must not be null or empty
 * 			2. input must not be null
 * 			3. inputLen must be greater than or equal to 0
 */
int base64_decode(char *output, char *input, int inputLen);

/* base64_enc_len:
 * 		Description:
 * 			Returns the length of a base64 encoded string whose decoded
 * 			form is inputLen bytes long
 * 		Parameters:
 * 			inputLen: the length of the decoded string
 * 		Return value:
 * 			The length of a base64 encoded string whose decoded form
 * 			is inputLen bytes long
 * 		Requirements:
 * 			None
 */
int base64_enc_len(int inputLen);

/* base64_dec_len:
 * 		Description:
 * 			Returns the length of the decoded form of a
 * 			base64 encoded string
 * 		Parameters:
 * 			input: the base64 encoded string to be measured
 * 			inputLen: the length of the base64 encoded string
 * 		Return value:
 * 			Returns the length of the decoded form of a
 * 			base64 encoded string
 * 		Requirements:
 * 			1. input must not be null
 * 			2. input must be greater than or equal to zero
 */
int base64_dec_len(char *input, int inputLen);

#endif // _BASE64_H
