
/** $VER: libriff.h (2025.03.14) P. Stuer **/

#pragma once

extern uint32_t __TRACE_LEVEL;

#ifdef __TRACE

#define TRACE_RESET()           { __TRACE_LEVEL = 0; }
#define TRACE_INDENT()          { ::printf("%*s{\n", __TRACE_LEVEL * 2, ""); __TRACE_LEVEL++; }
#define TRACE_FORM(type)        { char s[5]; *(uint32_t *) s = type; s[4] = 0; ::printf("%*sForm Type: \"%s\"\n", __TRACE_LEVEL * 2, "", s); }
#define TRACE_LIST(type, size)  { char s[5]; *(uint32_t *) s = type; s[4] = 0; ::printf("%*sList \"%s\", %u bytes\n", __TRACE_LEVEL * 2, "", s, size); }
#define TRACE_CHUNK(id, size)   { char s[5]; *(uint32_t *) s = id; s[4] = 0; ::printf("%*s%s, %u bytes\n", __TRACE_LEVEL * 2, "", s, size); }
#define TRACE_UNINDENT()        { __TRACE_LEVEL--; ::printf("%*s}\n", __TRACE_LEVEL * 2, ""); }

#else

#define TRACE_RESET()           {  }
#define TRACE_INDENT()          {  }
#define TRACE_FORM(type)        {  }
#define TRACE_LIST(type, size)  {  }
#define TRACE_CHUNK(id, size)   {  }
#define TRACE_UNINDENT()        {  }

#endif

#include "RIFFReader.h"
#include "Exception.h"
