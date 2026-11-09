#include "./lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

const SingleCharTokenEntry SINGLE_CHAR_TOKENS[] = {
  // Basic tokens
  {'(', TOKEN_SCLPAREN, "left parenthesis"},
  {')', TOKEN_SCRPAREN, "right parenthesis"},
  {'{', TOKEN_SCLBRACE, "left brace"},
  {'}', TOKEN_SCRBRACE, "right brace"},
  {'[', TOKEN_SCLBRACKET, "left bracket"},
  {']', TOKEN_SCRBRACKET, "right bracket"},
  {',', TOKEN_SCCOMMA, "comma"},
  {';', TOKEN_SCSEMICOLON, "semicolon"},
  {':', TOKEN_SCCOLON, "colon"},
  {'.', TOKEN_SCDOT, "dot"},
  
  // New single-character tokens
  {'#', TOKEN_SCHASH, "hash/length operator"},
  {'@', TOKEN_SCAT, "at/decorator"},
  {'?', TOKEN_SCQUESTION, "question/optional"},
  {'!', TOKEN_SCBANG, "bang/fallible"},
  {'^', TOKEN_SCCARET, "caret/tagged union"},
  {'|', TOKEN_SCPIPE, "pipe/untagged union"},
  {'&', TOKEN_SCAMP, "ampersand/constraint"},
  {'\'', TOKEN_SCQUOTE, "quote/lifetime"},
  {'"', TOKEN_SCDQUOTE, "double quote/string"},
  {'$', TOKEN_SCDOLLAR, "dollar/saturated"},
  {'~', TOKEN_SCTILDE, "tilde/bitwise not"},
  
  // Operators
  {'+', TOKEN_SCPLUS, "plus"},
  {'-', TOKEN_SCMINUS, "minus"},
  {'*', TOKEN_SCSTAR, "star/pointer"},
  {'/', TOKEN_SCSLASH, "slash/integer division"},
  {'%', TOKEN_SCPERCENT, "percent/modulo"},
  {'=', TOKEN_SCASSIGN, "equals/assignment"},
  {'<', TOKEN_SCLESS, "less"},
  {'>', TOKEN_SCGREATER, "greater"},
};

const size_t SINGLE_CHAR_TOKEN_COUNT = sizeof(SINGLE_CHAR_TOKENS) / sizeof(SINGLE_CHAR_TOKENS[0]);
