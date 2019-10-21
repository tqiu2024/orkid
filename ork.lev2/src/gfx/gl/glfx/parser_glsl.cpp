////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
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

/////////////////////////////////////////////////////////////////////////////////////////////////
namespace ork::lev2::glslfx {
/////////////////////////////////////////////////////////////////////////////////////////////////

std::string FnParseContext::tokenValue(size_t offset) const {
  return _view.token(_startIndex + offset)->text;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

int ParsedFunctionNode::parse(const ork::ScannerView& view) {
  int i         = 0;
  auto open_tok = view.token(i);
  assert(open_tok->text == "{");
  bool done = false;
  FnParseContext pctx(_container, view);
  while (not done) {
    auto try_tok     = view.token(i)->text;
    pctx._startIndex = i;
    if (auto m = VariableDeclaration::match(pctx)) {
      auto parsed = m.parse();
      i += parsed._numtokens;
      _elements.push_back(parsed._node);
    } else if (auto m = CompoundStatement::match(pctx)) {
      auto parsed = m.parse();
      i += parsed._numtokens;
      _elements.push_back(parsed._node);
    } else {
      assert(false);
    }
  }
  assert(false);
  auto close_tok = view.token(i++);
  assert(close_tok->text == "}");
  return i;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void ParsedFunctionNode::emit(ork::lev2::glslfx::shaderbuilder::BackEnd& backend) const {
  for (auto elem : _elements)
    elem->emit(backend);
  assert(false); // not implemented yet...
}

/////////////////////////////////////////////////////////////////////////////////////////////////

TypeName::match_t TypeName::match(FnParseContext ctx) {
  match_t rval(ctx);
  int count = 0;
  ////////////////////////////////////
  // check variable instantiation
  ////////////////////////////////////
  auto tokDT = ctx.tokenValue(count++);
  if (tokDT == "const") {
    tokDT = ctx.tokenValue(count++);
  }
  ////////////////////////////////////
  if (ctx._container->validateTypeName(tokDT)) {
    rval._matched = true;
    rval._start   = ctx._startIndex;
    rval._count   = count;
  }
  return rval;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

Identifier::match_t Identifier::match(FnParseContext ctx) {
  match_t rval(ctx);
  auto token = ctx.tokenValue(0);
  ////////////////////////////////////
  // check leading alphabetics's, _'s
  ////////////////////////////////////
  bool done = false;
  int count = 0;
  while( not done ){
    char ch = token[count];
    if( is_alf(ch) or (ch=='_' )){
      count++;
    }
    else
      done = true;
  }
  ////////////////////////////////////
  // check leading alphabetics's or numerics
  ////////////////////////////////////
  done = false;
  while( not done ){
    char ch = token[count];
    if( is_alf(ch) or (ch=='_') or is_num(ch) ){
      count++;
    }
    else
      done = true;
  }
  ////////////////////////////////////
  if (count) {
    rval._matched = true;
    rval._start   = ctx._startIndex;
    rval._count   = count;
  }
  return rval;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

VariableDeclaration::match_t VariableDeclaration::match(FnParseContext ctx) {
  match_t rval(ctx);
  if( auto m = TypeName::match(ctx)){
    if( ctx.tokenValue(m._count)==";"){
      rval._matched = true;
      rval._start   = ctx._startIndex;
      rval._count   = m._count+1;
    }
  }
  return rval;
}

VariableDeclaration::parsed_t VariableDeclaration::parse(const match_t& m) {
  parsed_t rval;
  assert(false);
  return rval;
}
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
}*/

/////////////////////////////////////////////////////////////////////////////////////////////////
/*
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

DeclarationList::match_t DeclarationList::match(FnParseContext ctx) {
  match_t rval(ctx);
  size_t count = 0;
  size_t start = -1;
  bool done    = false;
  while (not done) {
    auto mvd = VariableDeclaration::match(ctx);
    if (mvd) {
      count += mvd._count;
      count++; // consume }
      ctx = mvd.consume();
    } else {
      done = true;
    }
  }
  if (count) {
    rval._count   = count;
    rval._start   = start;
    rval._matched = true;
  }
  return rval;
}
DeclarationList::parsed_t DeclarationList::parse(const match_t& match) {
  parsed_t rval;
  return rval;
}
void DeclarationList::emit(shaderbuilder::BackEnd& backend) const {
  for (auto c : _children)
    c->emit(backend);
  assert(false);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2::glslfx
/////////////////////////////////////////////////////////////////////////////////////////////////
