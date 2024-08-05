#ifndef _PARSER_H_
#define _PARSER_H_

#include "lexer.h"
#include "json.h"

json parse(char *_txt, token *toks, struct json_options opts);

#endif
