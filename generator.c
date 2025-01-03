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
#ifndef GUARD_LIBJSON_GENERATOR_C
#define GUARD_LIBJSON_GENERATOR_C 1
#define _KOS_SOURCE 1

#include "api.h"
/**/

#include <__stdinc.h>

#include <hybrid/unaligned.h>

#include <format-printer.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include <libjson/generator.h>

#include "generator.h"
#include "parser.h"
#include "writer.h"

DECL_BEGIN

/*[[[config CONFIG_LIBJSON_GENERATOR_PRINTS_WARNINGS = true]]]*/
#ifdef CONFIG_NO_LIBJSON_GENERATOR_PRINTS_WARNINGS
#undef CONFIG_LIBJSON_GENERATOR_PRINTS_WARNINGS
#elif !defined(CONFIG_LIBJSON_GENERATOR_PRINTS_WARNINGS)
#define CONFIG_LIBJSON_GENERATOR_PRINTS_WARNINGS
#elif (-CONFIG_LIBJSON_GENERATOR_PRINTS_WARNINGS - 1) == -1
#undef CONFIG_LIBJSON_GENERATOR_PRINTS_WARNINGS
#define CONFIG_NO_LIBJSON_GENERATOR_PRINTS_WARNINGS
#endif /* ... */
/*[[[end]]]*/

#define DO(x)                           \
	do {                                \
		if unlikely((result = (x)) < 0) \
			goto done;                  \
	}	__WHILE0



#ifdef CONFIG_LIBJSON_GENERATOR_PRINTS_WARNINGS
#define MESSAGE(...) libjson_output_message(writer, __VA_ARGS__)
PRIVATE NONNULL((1, 2)) int CC
libjson_output_message(struct json_writer *__restrict writer,
                       char const *__restrict format, ...) {
	ssize_t temp;
	va_list args;
	va_start(args, format);
	if (writer->jw_result >= 0) {
#define DOPRINTER(x)                      \
		do {                              \
			if unlikely((temp = (x)) < 0) \
				goto err_printer;         \
			writer->jw_result += temp;    \
		}	__WHILE0
		DOPRINTER((*writer->jw_printer)(writer->jw_arg, "/* ", 3));
		DOPRINTER(format_vprintf(writer->jw_printer,
		                         writer->jw_arg,
		                         format,
		                         args));
		DOPRINTER((*writer->jw_printer)(writer->jw_arg, " */", 3));
#undef DOPRINTER
	}
	va_end(args);
	return 0;
err_printer:
	va_end(args);
	writer->jw_result = temp;
	return -1;
}
#else /* CONFIG_LIBJSON_GENERATOR_PRINTS_WARNINGS */
#define MESSAGE(...) (void)0
#endif /* !CONFIG_LIBJSON_GENERATOR_PRINTS_WARNINGS */


#define GENFLAG_NORMAL   0x0000
#define GENFLAG_OPTIONAL 0x0001


PRIVATE NONNULL((1, 2, 3, 4)) int CC
libjson_encode_INTO(struct json_writer *__restrict writer,
                    byte_t const **__restrict preader,
                    void const *__restrict src_base,
                    void const *__restrict src,
                    uint8_t type, unsigned int gen_flags) {
	int result;
	(void)gen_flags;
	switch (type) {

	case JSON_TYPE_BOOLBIT(0) ... JSON_TYPE_BOOLBIT(7): {
		uint8_t value;
		uint8_t mask;
		value = *(uint8_t const *)src;
		mask  = (uint8_t)1 << (type - JSON_TYPE_BOOLBIT(0));
		result = libjson_writer_putbool(writer, (value & mask) != 0);
	}	break;

	case JSON_TYPE_INT8:
		result = libjson_writer_putnumber(writer, (int8_t)UNALIGNED_GET8(src));
		break;

	case JSON_TYPE_INT16:
		result = libjson_writer_putnumber(writer, (int16_t)UNALIGNED_GET16(src));
		break;

	case JSON_TYPE_INT32:
		result = libjson_writer_putnumber(writer, (int32_t)UNALIGNED_GET32(src));
		break;

	case JSON_TYPE_INT64:
		result = libjson_writer_putint64(writer, (int64_t)UNALIGNED_GET64(src));
		break;

	case JSON_TYPE_UINT8:
		result = libjson_writer_putnumber(writer, (intptr_t)(uintptr_t)UNALIGNED_GET8(src));
		break;

	case JSON_TYPE_UINT16:
		result = libjson_writer_putnumber(writer, (intptr_t)(uintptr_t)UNALIGNED_GET16(src));
		break;

	case JSON_TYPE_UINT32:
		result = libjson_writer_putuint64(writer, UNALIGNED_GET32(src));
		break;

	case JSON_TYPE_UINT64:
		result = libjson_writer_putuint64(writer, UNALIGNED_GET64(src));
		break;

#ifndef __NO_FPU
	case JSON_TYPE_FLOAT: {
		float value;
		memcpy(&value, src, sizeof(value));
		result = libjson_writer_putfloat(writer, (double)value);
	}	break;

	case JSON_TYPE_DOUBLE: {
		double value;
		memcpy(&value, src, sizeof(value));
		result = libjson_writer_putfloat(writer, value);
	}	break;

#ifdef __COMPILER_HAVE_LONGLONG
	case JSON_TYPE_LDOUBLE: {
		__LONGDOUBLE value;
		memcpy(&value, src, sizeof(value));
		result = libjson_writer_putfloat(writer, (double)value);
	}	break;
#endif /* __COMPILER_HAVE_LONGLONG */
#endif /* !__NO_FPU */

	case JSON_TYPE_INLINE_STRING_OP: {
		char const *str;
		uint16_t len;
		len = UNALIGNED_GET16(*preader);
		*preader += 2;
		str = (char const *)src;
		/* Truncate the string to exclude trailing zeroes. */
		while (len && str[len - 1] == 0)
			--len;
		/* Write the string. */
		result = libjson_writer_putstring(writer, str, len);
	}	break;

	case JSON_TYPE_STRING:
	case JSON_TYPE_STRING_OR_NULL:
	case JSON_TYPE_STRING_WITH_LENGTH_OP: {
		char *str;
		size_t len;
		str = (char *)UNALIGNED_GET((uintptr_t *)src);
		if (type == JSON_TYPE_STRING_WITH_LENGTH_OP) {
			uint16_t len_offset;
			len_offset = UNALIGNED_GET16(*preader);
			*preader += 2;
			len = UNALIGNED_GET((size_t const *)((byte_t const *)src_base + len_offset));
		} else if (type == JSON_TYPE_STRING_OR_NULL && str == NULL) {
			result = libjson_writer_putnull(writer);
			break;
		} else {
			len = strlen(str);
		}
		result = libjson_writer_putstring(writer, str, len);
	}	break;

	case JSON_TYPE_VOID:
		result = libjson_writer_putnull(writer);
		break;

	default:
		MESSAGE("Illegal type `%#.2" PRIx8 "' in INTO", type);
		result = -2;
		break;
	}
	return result;
}



/* Process until _after_ the correct `JGEN_END' opcode is reached. */
INTDEF NONNULL((1, 2, 3)) int CC
libjson_encode_OBJECT(struct json_writer *__restrict writer,
                      byte_t const **__restrict preader,
                      void const *__restrict src,
                      unsigned int gen_flags,
                      void const *const *ext);

/* Process until _after_ the correct `JGEN_END' opcode is reached. */
INTDEF NONNULL((1, 2, 3)) int CC
libjson_encode_ARRAY(struct json_writer *__restrict writer,
                     byte_t const **__restrict preader,
                     void const *__restrict src,
                     unsigned int gen_flags,
                     void const *const *ext);


/* Process the designator that must follow one of the following opcodes:
 *   - JGEN_FIELD
 *   - JGEN_INDEX
 *   - JGEN_INDEX_REP_OP
 * This designator can be one of the following:
 *   - JGEN_BEGINOBJECT
 *   - JGEN_BEGINARRAY
 *   - JGEN_INTO
 */
PRIVATE NONNULL((1, 2, 3)) int CC
libjson_encode_designator(struct json_writer *__restrict writer,
                          byte_t const **__restrict preader,
                          void const *__restrict src,
                          unsigned int gen_flags,
                          void const *const *ext) {
	int result;
	byte_t op;
	byte_t const *reader = *preader;
	op = *reader++;
	switch (op) {

	case JGEN_BEGINOBJECT:
		result = libjson_encode_OBJECT(writer, (byte_t const **)&reader, src, gen_flags, ext);
		break;

	case JGEN_BEGINARRAY:
		result = libjson_encode_ARRAY(writer, (byte_t const **)&reader, src, gen_flags, ext);
		break;

	case JGEN_INTO: {
		uint16_t offset;
		uint8_t type;
		offset  = UNALIGNED_GET16(reader);
		reader += 2;
		type    = *(uint8_t const *)reader;
		reader += 1;
		result = libjson_encode_INTO(writer, &reader, src, (byte_t const *)src + offset, type, gen_flags);
	}	break;

	case JGEN_EXTERN_OP: {
		uint16_t offset, id;
		offset = UNALIGNED_GET16(reader);
		reader += 2;
		id = UNALIGNED_GET16(reader);
		reader += 2;
		result = libjson_encode(writer, ext[id], (byte_t const *)src + offset, ext);
	}	break;

	default:
		MESSAGE("Illegal instruction `%#.2" PRIx8 "' in DESIGNATOR", op);
		writer->jw_state = JSON_WRITER_STATE_BADUSAGE;
		result = -2;
		break;
	}
	*preader = reader;
	return result;
}


INTERN NONNULL((1, 2, 3)) int CC
libjson_encode_OBJECT(struct json_writer *__restrict writer,
                      byte_t const **__restrict preader,
                      void const *__restrict src,
                      unsigned int gen_flags,
                      void const *const *ext) {
	int result;
	byte_t op;
	byte_t const *reader = *preader;
	DO(libjson_writer_beginobject(writer));
	for (;;) {
		unsigned int inner_flags = 0;
again_inner:
		op = *reader++;
		switch (op) {

		case JGEN_END:
			goto done_object;

		case JGEN_FIELD: {
			size_t namelen;
			namelen = strlen((char *)reader);
			DO(libjson_writer_addfield(writer, (char *)reader, namelen));
			reader += namelen + 1;
			/* parse the field designator. */
			DO(libjson_encode_designator(writer, &reader, src,
				                         inner_flags | (gen_flags & ~GENFLAG_OPTIONAL),
			                             ext));
		}	break;

		case JGEN_OPTIONAL:
			if unlikely(inner_flags & GENFLAG_OPTIONAL) {
				MESSAGE("JGEN_OPTIONAL used more than once");
				goto err_bad_usage;
			}
			inner_flags |= GENFLAG_OPTIONAL;
			goto again_inner;

		default:
			MESSAGE("Illegal instruction `%#.2" PRIx8 "' in OBJECT", op);
err_bad_usage:
			writer->jw_state = JSON_WRITER_STATE_BADUSAGE;
			result = -2;
			goto done;
		}
	}
done_object:
	DO(libjson_writer_endobject(writer));
done:
	*preader = reader;
	return result;
}


INTERN NONNULL((1, 2, 3)) int CC
libjson_encode_ARRAY(struct json_writer *__restrict writer,
                     byte_t const **__restrict preader,
                     void const *__restrict src,
                     unsigned int gen_flags,
                     void const *const *ext) {
	int result;
	byte_t op;
	byte_t const *reader = *preader;
	uint16_t next_index = 0;
	DO(libjson_writer_beginarray(writer));
	for (;;) {
		unsigned int inner_flags = 0;
again_inner:
		op = *reader++;
		switch (op) {

		case JGEN_END:
			goto done_array;

		case JGEN_INDEX_OP: {
			uint16_t index;
			index = UNALIGNED_GET16(reader);
			reader += 2;
			/* Make sure that indices only ever increment! */
			if unlikely(index < next_index)
				goto err_bad_usage;
			while (next_index < index) {
				/* Fill unused indices with `null' */
				DO(libjson_writer_putnull(writer));
				++next_index;
			}
			/* Create the array element designator. */
			DO(libjson_encode_designator(writer, &reader, src,
			                             inner_flags | (gen_flags & ~GENFLAG_OPTIONAL),
			                             ext));
			++next_index; /* Account for the written index. */
		}	break;

		case JGEN_INDEX_REP_OP: {
			uint16_t count;
			uint16_t stride;
			size_t offset;
			byte_t const *orig_reader;
			count   = UNALIGNED_GET16(reader) + 1;
			reader += 2;
			stride  = UNALIGNED_GET16(reader);
			reader += 2;
			offset  = 0;
			orig_reader = reader;
			do {
				/* parse the array element designator. */
				reader = orig_reader;
				DO(libjson_encode_designator(writer,
				                             &reader,
				                             (byte_t const *)src + offset,
				                             inner_flags | (gen_flags & ~GENFLAG_OPTIONAL),
				                             ext));
				offset += stride;
			} while (--count != 0);
		}	break;

		case JGEN_OPTIONAL:
			if unlikely(inner_flags & GENFLAG_OPTIONAL) {
				MESSAGE("JGEN_OPTIONAL used more than once");
				goto err_bad_usage;
			}
			inner_flags |= GENFLAG_OPTIONAL;
			goto again_inner;

		default:
			MESSAGE("Illegal instruction `%#.2" PRIx8 "' in OBJECT", op);
err_bad_usage:
			writer->jw_state = JSON_WRITER_STATE_BADUSAGE;
			result = -2;
			goto done;
		}
	}
done_array:
	DO(libjson_writer_endarray(writer));
done:
	*preader = reader;
	return result;
}


/* Encode a given object `src' as Json, using the given `codec' as generator.
 * @param: ext: External codec vector (s.a. `JGEN_EXTERN(...)')
 * @return:  0: Success.
 * @return: -1: Error: `writer->jw_result' has a negative value when the function was called.
 * @return: -1: Error: An invocation of the `writer->jw_printer' returned a negative value.
 * @return: -2: Error: Invalid usage during this, or during an earlier call. */
INTERN NONNULL((1, 2, 3)) int CC
libjson_encode(struct json_writer *__restrict writer,
               void const *__restrict codec,
               void const *__restrict src,
               void const *const *ext) {
	int result;
	byte_t op;
	byte_t const *reader = (byte_t const *)codec;
	unsigned int gen_flags = GENFLAG_NORMAL;
again_parseop:
	op = *reader++;
	switch (op) {

	case JGEN_TERM:
		result = 0;
		break;

	case JGEN_BEGINOBJECT:
		DO(libjson_encode_OBJECT(writer, (byte_t const **)&reader, src, gen_flags, ext));
require_term:
		op = *reader++;
		if unlikely(op != JGEN_TERM) {
			MESSAGE("Missing JGEN_TERM (got `%#.2" PRIx8 "' at offset `%" PRIdSIZ "')",
			        op, (reader - 1) - (byte_t *)codec);
			goto err_bad_usage;
		}
		break;

	case JGEN_BEGINARRAY:
		DO(libjson_encode_ARRAY(writer, (byte_t const **)&reader, src, gen_flags, ext));
		goto require_term;

	case JGEN_EXTERN_OP: {
		uint16_t offset, id;
		offset = UNALIGNED_GET16(reader);
		reader += 2;
		id = UNALIGNED_GET16(reader);
		reader += 2;
		result = libjson_encode(writer, ext[id], (byte_t const *)src + offset, ext);
		goto require_term;
	}	break;

	case JGEN_OPTIONAL:
		if unlikely(gen_flags & GENFLAG_OPTIONAL) {
			MESSAGE("JGEN_OPTIONAL used more than once");
			goto err_bad_usage;
		}
		gen_flags |= GENFLAG_OPTIONAL;
		goto again_parseop;

	case JGEN_INTO: {
		uint16_t offset;
		uint8_t type;
		offset  = UNALIGNED_GET16(reader);
		reader += 2;
		type    = *(uint8_t const *)reader;
		reader += 1;
		DO(libjson_encode_INTO(writer, &reader, src, (byte_t const *)src + offset, type, gen_flags));
		goto require_term;
	}	break;

	default:
		MESSAGE("Illegal primary instruction `%#.2" PRIx8 "'", op);
err_bad_usage:
		writer->jw_state = JSON_WRITER_STATE_BADUSAGE;
		result = -2;
		goto done;
	}
done:
	return result;
}
#undef DO
#undef MESSAGE



#ifndef __INTELLISENSE__
DECL_END

#define MODE_ZERO 1
#include "generator-decode-impl.c.inl"

#define MODE_DECODE 1
#include "generator-decode-impl.c.inl"

DECL_BEGIN
#endif /* !__INTELLISENSE__ */




INTDEF NONNULL((1, 2, 3, 4)) int
NOTHROW_NCX(CC libjson_decode_INTO)(struct json_parser *__restrict parser,
                                    byte_t const **__restrict preader,
                                    void *__restrict dst_base,
                                    void *__restrict dst,
                                    uint8_t type,
                                    unsigned int gen_flags);

/* Process until _after_ the correct `JGEN_END' opcode is reached. */
INTDEF NONNULL((1, 2, 3)) int
NOTHROW_NCX(CC libjson_decode_OBJECT)(struct json_parser *__restrict parser,
                                      byte_t const **__restrict preader,
                                      void *__restrict dst,
                                      unsigned int gen_flags,
                                      void const *const *ext);

/* Process until _after_ the correct `JGEN_END' opcode is reached. */
INTDEF NONNULL((1, 2, 3)) int
NOTHROW_NCX(CC libjson_decode_ARRAY)(struct json_parser *__restrict parser,
                                     byte_t const **__restrict preader,
                                     void *__restrict dst,
                                     unsigned int gen_flags,
                                     void const *const *ext);




/* Decode a given object `dst' from Json, using the given `codec' as generator.
 * @param: ext: External codec vector (s.a. `JGEN_EXTERN(...)')
 * @return: JSON_ERROR_OK:     Success.
 * @return: JSON_ERROR_SYNTAX: Syntax error.
 * @return: JSON_ERROR_NOOBJ:  A required field doesn't exist or has wrong typing.
 * @return: JSON_ERROR_SYSERR: Malformed codec.
 * @return: JSON_ERROR_RANGE:  The value of an integer field was too large. */
INTERN NONNULL((1, 2, 3)) int
NOTHROW_NCX(CC libjson_decode)(struct json_parser *__restrict parser,
                               void const *__restrict codec,
                               void *__restrict dst,
                               void const *const *ext) {
	int result;
	byte_t op;
	byte_t const *reader = (byte_t *)codec;
	unsigned int gen_flags = GENFLAG_NORMAL;
again_parseop:
	op = *reader++;
	switch (op) {

	case JGEN_TERM:
		result = 0;
		break;

	case JGEN_BEGINOBJECT:
		result = libjson_decode_OBJECT(parser, (byte_t const **)&reader, dst, gen_flags, ext);
require_term:
		if (result == JSON_ERROR_OK) {
			op = *reader++;
			if unlikely(op != JGEN_TERM)
				goto err_bad_usage;
		}
		break;

	case JGEN_BEGINARRAY:
		result = libjson_decode_ARRAY(parser, (byte_t const **)&reader, dst, gen_flags, ext);
		goto require_term;

	case JGEN_OPTIONAL:
		if unlikely(gen_flags & GENFLAG_OPTIONAL)
			goto err_bad_usage;
		gen_flags |= GENFLAG_OPTIONAL;
		goto again_parseop;

	case JGEN_INTO: {
		uint16_t offset;
		uint8_t type;
		offset  = UNALIGNED_GET16(reader);
		reader += 2;
		type    = *(uint8_t const *)reader;
		reader += 1;
		result = libjson_decode_INTO(parser, &reader, dst, (byte_t *)dst + offset, type, gen_flags);
		goto require_term;
	}	break;

	case JGEN_EXTERN_OP: {
		/* Allowed but highly unlikely: The base codec reference one that is external */
		uint16_t offset, id;
		offset = UNALIGNED_GET16(reader);
		reader += 2;
		id = UNALIGNED_GET16(reader);
		reader += 2;
		/* Make sure that the base codec ends after a single primary object! */
		if unlikely(*reader != JGEN_TERM)
			goto err_bad_usage;
		/* Switch to the external codec. */
		reader = (byte_t const *)ext[id];
		dst    = (byte_t *)dst + offset;
		goto again_parseop;
	}	break;

	default:
err_bad_usage:
		result = JSON_ERROR_SYSERR;
		break;
	}
	return result;
}

DEFINE_PUBLIC_ALIAS(json_encode, libjson_encode);
DEFINE_PUBLIC_ALIAS(json_decode, libjson_decode);

DECL_END

#endif /* !GUARD_LIBJSON_GENERATOR_C */
