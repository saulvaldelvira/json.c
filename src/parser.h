#ifndef _PARSER_H_
#define _PARSER_H_

#include "lexer.h"
#include "json.h"

json_t parse(char *_txt, token *toks, struct json_options opts, bool validate);

#endif
