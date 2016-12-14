/*
 * Copyright (C) 2016 Hops.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/* 
 * File:   Utils.h
 * Author: Mahmoud Ismail<maism@kth.se>, Konstantin Popov <kost@sics.se>
 *
 */

#ifndef UTILS_H
#define UTILS_H

#include "common.h"
#include <cstdlib>
#include <cstring>
#include <sstream>
#include "EventMsg.h"

using namespace std;
namespace Utils {

    namespace NdbC {

      
        inline static string get_ndb_varchar(string str, NdbDictionary::Column::ArrayType array_type) {
            stringstream data;
            int len = str.length();

            switch (array_type) {
                case NdbDictionary::Column::ArrayTypeFixed:
                    /*
                     No prefix length is stored in aRef. Data starts from aRef's first byte
                     data might be padded with blank or null bytes to fill the whole column
                     */
                    data << str;
                    break;
                case NdbDictionary::Column::ArrayTypeShortVar:
                    /*
                     First byte of aRef has the length of data stored
                     Data starts from second byte of aRef
                     */
                    data << ((char) len) << str;
                    break;
                case NdbDictionary::Column::ArrayTypeMediumVar:
                    /*
                     First two bytes of aRef has the length of data stored
                     Data starts from third byte of aRef
                     */
                    int m = len / 256;
                    int l = len - (m * 256);
                    data << ((char) l) << ((char) m) << str;
                    break;
            }
            return data.str();
        }

        /*
         * Based on The example file ndbapi_array_simple.cpp found in 
         * the MySQL Cluster source distribution's storage/ndb/ndbapi-examples directory
         */

        /* extracts the length and the start byte of the data stored */
      /// the data container is owned by ndb, that is, it must be
      /// saved somewhere else if not used immediately;
        inline static int get_byte_array(const NdbRecAttr* attr,
                const char*& first_byte,
                size_t& bytes) {
            const NdbDictionary::Column::ArrayType array_type =
                    attr->getColumn()->getArrayType();
            const size_t attr_bytes = attr->get_size_in_bytes();
            const char* aRef = attr->aRef();

            switch (array_type) {
                case NdbDictionary::Column::ArrayTypeFixed:
                    /*
                     No prefix length is stored in aRef. Data starts from aRef's first byte
                     data might be padded with blank or null bytes to fill the whole column
                     */
                    first_byte = aRef;
                    bytes = attr_bytes;
                    return 0;
                case NdbDictionary::Column::ArrayTypeShortVar:
                    /*
                     First byte of aRef has the length of data stored
                     Data starts from second byte of aRef
                     */
                    first_byte = aRef + 1;
                    bytes = (size_t) (aRef[0]);
                    return 0;
                case NdbDictionary::Column::ArrayTypeMediumVar:
                    /*
                     First two bytes of aRef has the length of data stored
                     Data starts from third byte of aRef
                     */
                    first_byte = aRef + 2;
                    bytes = (size_t) (aRef[1]) * 256 + (size_t) (aRef[0]);
                    return 0;
                default:
                    first_byte = NULL;
                    bytes = 0;
                    return -1;
            }
        }

        /*
         Extracts the string from given NdbRecAttr
         Uses get_byte_array internally
         */
        inline static string get_string(const NdbRecAttr* attr) {
            size_t attr_bytes;
            const char* data_start_ptr = NULL;

            /* get stored length and data using get_byte_array */
            if (get_byte_array(attr, data_start_ptr, attr_bytes) == 0) {
                /* we have length of the string and start location */
                string str = string(data_start_ptr, attr_bytes);
                if (attr->getType() == NdbDictionary::Column::Char) {
                    /* Fixed Char : remove blank spaces at the end */
                    size_t endpos = str.find_last_not_of(" ");
                    if (string::npos != endpos) {
                        str = str.substr(0, endpos + 1);
                    }
                }
                return string(str);
            }
            return NULL;
        }

      /// Extracts the (native C) string from given NdbRecAttr
      /// Uses get_byte_array internally
      /// Allocates memory using either EventMsg::calloc()
      inline static char* const get_cstring(const NdbRecAttr* const attr, EventMsg *msg)
      {
	size_t attr_bytes;
	const char* data_start_ptr = NULL;

	/* get stored length and data using get_byte_array */
	if (get_byte_array(attr, data_start_ptr, attr_bytes) == 0) {
	  /* we have length of the string and start location */
	  if (attr->getType() == NdbDictionary::Column::Char) {
	    /* Fixed Char : remove blank spaces at the end */
	    for (int nonCharIdx = attr_bytes - 1 ; nonCharIdx >= 0; nonCharIdx--)
	      if (data_start_ptr[nonCharIdx] == ' ')
		attr_bytes--;
	  }

	  // use EventMsg's "own" heap
	  char *cstr = msg->calloc(attr_bytes + 1); // including the trailing \0;
	  /*
	   * do NOT fallback for malloc() for large chunks: apparently
	   * (comparing the execution times of a special test program
	   * doing malloc()"s of different sizes) these are not
	   * free-listed and cause excessive brk() system calls, while
	   * allocating memory that is seldom used (but cover
	   * worst-case requirements) actually causes little, if any
	   * performance degradation. Furthermore, having two distinct
	   * sources of memory - through EventMsg::calloc() and
	   * malloc() - would require explicit message inspection and
	   * appropriate free() calls when messages are reclaimed;
	   *
	  if (cstr == (char *) NULL)
	    // fallback to native malloc() - at the price of the cost
	    // of the latter, now and later on when EventMsg is
	    // reclaimed, and all malloc() -allocated strings must be free() 'd;
	    cstr = malloc(attr_bytes + 1);
	  if (cstr == (char *) NULL) {
	    LOG_ERROR("get_cstring() run out of memory");
	    exit(1);
	  }
	  */
	  assert(cstr != (char *) NULL);
	      
	  //
	  (void) memcpy(cstr, data_start_ptr, attr_bytes);
	  cstr[attr_bytes] = 0;
	  return (cstr);
	} else {
	  return ((char *) NULL);
	}
      }

    inline static string concat(const char* a, const string b) {
        string buf(a);
        buf.append(b);
        return buf;
    }
}

#endif /* UTILS_H */

