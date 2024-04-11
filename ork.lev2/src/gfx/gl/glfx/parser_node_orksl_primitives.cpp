////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
//  Scanner/Parser
//  this replaces CgFx for OpenGL 3.x and OpenGL ES 2.x
////////////////////////////////////////////////////////////////

#include "../gl.h"
#include "glslfxi.h"
#include "glslfxi_parser.h"
#include <ork/file/file.h>
#include <ork/lev2/gfx/gfxenv.h>
#include <ork/pch.h>
#include <ork/util/crc.h>
#include <regex>
#include <stdlib.h>

#if defined(USE_ORKSL_LANG)

/////////////////////////////////////////////////////////////////////////////////////////////////
namespace ork::lev2::glslfx::parser {
/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t Constant::match(FnParseContext ctx) {
  match_results_t rval;
  auto token = ctx.tokenValue(0);
  ////////////////////////////////////
  // boolean constants
  ////////////////////////////////////
  if (token == "true" or token == "false") {
    rval           = std::make_shared<match_t>(ctx);
    rval->_matched = true;
    rval->_start   = ctx._startIndex;
    rval->_count   = token.length();
    return rval;
  }
  ////////////////////////////////////
  // numeric constants, check leading digits
  ////////////////////////////////////
  bool done = false;
  int count = 0;
  char ch;
  while (not done) {
    ch = token[count];
    if (is_num(ch)) {
      count++;
    } else
      done = true;
  }
  ////////////////////////////////////
  // check decimal point
  ////////////////////////////////////
  ch = token[count];
  if (ch == '.')
    count++;
  ////////////////////////////////////
  // check following digits
  ////////////////////////////////////
  done = false;
  while (not done) {
    ch = token[count];
    if (is_num(ch)) {
      count++;
    } else
      done = true;
  }
  ////////////////////////////////////
  // early termination
  ////////////////////////////////////
  if (0 == count)
    return rval;
  ////////////////////////////////////
  // check e (scientific notation)
  ////////////////////////////////////
  ch = token[count];
  if (ch == 'e')
    count++;
  ch = token[count];
  if (ch == '-' or ch == '+')
    count++;
  done = false;
  while (not done) {
    ch = token[count];
    if (is_num(ch)) {
      count++;
    } else
      done = true;
  }
  ////////////////////////////////////
  if (count) {
    rval           = std::make_shared<match_t>(ctx);
    rval->_matched = true;
    rval->_start   = ctx._startIndex;
    rval->_count   = 1; // number of tokens, not characters
  }
  return rval;

  return rval;
};

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t DeclarationList::match(FnParseContext ctx) {
  match_results_t rval;
  size_t count = 0;
  size_t start = -1;
  bool done    = false;
  while (not done) {
    auto mvd = VariableDeclaration::match(ctx);
    if (mvd) {
      count += mvd->_count;
      count++; // consume }
      ctx = mvd->consume();
    } else {
      done = true;
    }
  }
  if (count) {
    rval           = std::make_shared<match_t>(ctx);
    rval->_count   = count;
    rval->_start   = start;
    rval->_matched = true;
  }
  return rval;
}
// DeclarationList::parsed_t DeclarationList::parse(const match_t& match) {
// parsed_t rval;
// return rval;
//}
void DeclarationList::emit(shaderbuilder::BackEnd& backend) const {
  for (auto c : _children)
    c->emit(backend);
  assert(false);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t Identifier::match(FnParseContext ctx) {
  match_results_t rval;
  auto token = ctx.tokenValue(0);
  if (token == "nrm") {
  }
  auto topnode = ctx._parser->_topNode;
  if (topnode->validateKeyword(token)) { // an identifer cannot be a keyword
    return rval;
  }
  ////////////////////////////////////
  // check leading alfa's or _'s
  ////////////////////////////////////
  bool done = false;
  int count = 0;
  while (not done) {
    char ch = token[count];
    if (is_alf(ch) or (ch == '_')) {
      count++;
    } else
      done = true;
  }
  ////////////////////////////////////
  // check leading alfnums or _'s
  ////////////////////////////////////
  done = (count == 0);
  while (not done) {
    char ch = token[count];
    if (is_alfnum(ch) or (ch == '_')) {
      count++;
    } else
      done = true;
  }
  ////////////////////////////////////
  if (count) {
    rval           = std::make_shared<match_t>(ctx);
    rval->_matched = true;
    rval->_start   = ctx._startIndex;
    rval->_count   = 1;
  }
  return rval;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t IdentifierPath::match(FnParseContext ctx) {
  match_results_t rval;
  bool done = false;
  while (not done) {

    if (auto mi = Identifier::match(ctx)) {
      if (not rval) {
        rval = std::make_shared<match_t>(ctx);
      }
      rval = rval + mi;
      ctx  = rval->consume();
      if (auto md = DotOp::match(ctx)) {
        rval = rval + md;
        ctx  = rval->consume();
      }
    } else
      done = true;
  }
  return rval;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t Reference::match(FnParseContext ctx) {
  match_results_t rval;
  if (auto mi = IdentifierPath::match(ctx)) {
    if (not rval) {
      rval = std::make_shared<match_t>(ctx);
    }
    rval = rval + mi;
    ctx  = rval->consume();
    if (auto mo = OpenSquare::match(ctx)) {
      rval    = rval + mo;
      ctx     = rval->consume();
      auto me = Expression::match(ctx);
      assert(me);
      rval    = rval + me;
      ctx     = rval->consume();
      auto mc = CloseSquare::match(ctx);
      assert(mc);
      rval = rval + mc;
      ctx  = rval->consume();
    }
  }
  return rval;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t StringLiteral::match(FnParseContext ctx) {
  match_results_t rval;
  // std::make_shared<match_t>(ctx);
  // dont need these yet, GLSL does not natively support them
  return rval;
};

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t TypeName::match(FnParseContext ctx) {
  match_results_t rval;
  int count = 0;
  ////////////////////////////////////
  // check variable instantiation
  ////////////////////////////////////
  auto tokDT = ctx.tokenValue(count++);
  if (tokDT == "const") {
    tokDT = ctx.tokenValue(count++);
  }
  ////////////////////////////////////
  auto topnode = ctx._parser->_topNode;
  if (topnode->isTypeName(tokDT)) {
    rval           = std::make_shared<match_t>(ctx);
    rval->_matched = true;
    rval->_start   = ctx._startIndex;
    rval->_count   = count;
  }
  return rval;
}


/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t VariableDeclaration::match(FnParseContext ctx) {
  match_results_t rval;
  if (auto m = TypeName::match(ctx)) {
    if (ctx.tokenValue(m->_count) == ";") {
      rval           = std::make_shared<match_t>(ctx);
      rval->_matched = true;
      rval->_start   = ctx._startIndex;
      rval->_count   = m->_count + 1;
    }
  }
  return rval;
}

// parsed_t VariableDeclaration::parse(const match_t& m) {
// parsed_t rval;
// assert(false);
// return rval;
//}
void VariableDeclaration::emit(shaderbuilder::BackEnd& backend) const {
  assert(false);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

/*
FnMatchResults VariableDefinitionStatement::match(const FnParseContext& ctx) {
  FnMatchResults rval;
  ////////////////////////////////////
  // check variable instantiation
  ////////////////////////////////////
  int i      = 0;
  auto tokDT = ctx.tokenValue(i);
  if (tokDT == "const") {
    tokDT = ctx.tokenValue(++i);
  }
  bool valid_dt = ctx._container->validateTypeName(tokDT);
  auto tokN     = ctx.tokenValue(i + 1);
  bool valid_id = ctx._container->validateIdentifierName(tokN);
  auto tokE     = ctx.tokenValue(i + 2);
  bool instantiation_ok = valid_dt and valid_id and (tokE == "=");
  if( false == instantiation_ok )
    return rval;
  ////////////////////////////////////
  // check assignment
  ////////////////////////////////////
  FnParseContext lctx = ctx;
  lctx._startIndex = i+1;
  auto matchlv = LValue::match(ctx);
  if( matchlv ){
    auto try_eq = ctx.tokenValue(matchlv._end+1);
    if( try_eq=="="){
      FnParseContext rctx = ctx;
      rctx._startIndex = matchlv._end+2;
      auto matchrv = RValue::match(ctx);
      if( matchrv ) {
        rval._matched = true;
        rval._start = ctx._startIndex;
        rval._end = matchrv._end;
      }
    }
  }
  ////////////////////////////////////
  return rval;
}

int VariableDefinitionStatement::parse(const FnParseContext& ctx, const FnMatchResults& r) {
  assert(false);
  return 0;
}
void VariableDefinitionStatement::emit(shaderbuilder::BackEnd& backend) const {
  assert(false);
}
/////////////////////////////////////////////////////////////////////////////////////////////////

FnMatchResults VariableAssignmentStatement::match(const FnParseContext& ctx) {
  FnMatchResults rval;
  auto matchlv = LValue::match(ctx);
  if( matchlv ){
    auto try_eq = ctx.tokenValue(matchlv._end+1);
    if( try_eq=="="){
      FnParseContext rctx = ctx;
      rctx._startIndex = matchlv._end+2;
      auto matchrv = RValue::match(ctx);
      if( matchrv ) {
        rval._matched = true;
        rval._start = ctx._startIndex;
        rval._end = matchrv._end;
      }
    }
  }
  return rval;
}

int VariableAssignmentStatement::parse(const FnParseContext& ctx, const FnMatchResults& r) {
  assert(false);
  return 0;
}
void VariableAssignmentStatement::emit(shaderbuilder::BackEnd& backend) const {
  assert(false);
}
*/

/////////////////////////////////////////////////////////////////////////////////////////////////
} //namespace ork::lev2::glslfx::parser {
/////////////////////////////////////////////////////////////////////////////////////////////////

#endif