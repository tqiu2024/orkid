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
#include <peglib.h>
#include <ork/util/logger.h>
#include <ork/kernel/string/string.h>

#if defined(USE_ORKSL_LANG)

/////////////////////////////////////////////////////////////////////////////////////////////////
namespace ork::lev2::glslfx::parser {
/////////////////////////////////////////////////////////////////////////////////////////////////

static logchannel_ptr_t logchan = logger()->createChannel("ORKSLIMPL",fvec3(1,1,.9),true);
static logchannel_ptr_t logchan_grammar = logger()->createChannel("ORKSLGRAM",fvec3(1,1,.8),true);
static logchannel_ptr_t logchan_lexer = logger()->createChannel("ORKSLLEXR",fvec3(1,1,.7),true);

struct ScannerLightView{
  ScannerLightView(const ScannerView& inp_view)
    : _input_view(inp_view)
    , _start(inp_view._start)
    , _end(inp_view._end)
  {
  }
  ScannerLightView(const ScannerLightView& oth)
    : _input_view(oth._input_view)
    , _start(oth._start)
    , _end(oth._end)
  {
  }
  const Token* token(size_t i) const{
    return _input_view.token(i);
  }
  TokenClass token_class(size_t i) const{
    auto tok = token(i);
    return (TokenClass) tok->_class;
  }
  const ScannerView& _input_view;
  size_t _start = -1; 
  size_t _end = -1;   
};
using scannerlightview_ptr_t = std::shared_ptr<ScannerLightView>;
using scannerlightview_constptr_t = std::shared_ptr<const ScannerLightView>;
using matcher_fn_t = std::function<scannerlightview_ptr_t(scannerlightview_constptr_t& inp_view)>;

//////////////////////////////////////////////////////////////

struct Matcher {
  Matcher(matcher_fn_t match_fn)
    : _match_fn(match_fn) {
  }
  scannerlightview_ptr_t match(scannerlightview_constptr_t inp_view) const {
    return _match_fn(inp_view);
  }
  matcher_fn_t _match_fn;
};

using matcher_ptr_t = std::shared_ptr<Matcher>;

//////////////////////////////////////////////////////////////

#define MATCHER(x) auto x = createMatcher([=](scannerlightview_constptr_t inp_view)->scannerlightview_ptr_t

//////////////////////////////////////////////////////////////

struct OrkslPEG{

  OrkslPEG(){

    auto equals = matcherForTokenClass(TokenClass::EQUALS);
    auto semicolon = matcherForTokenClass(TokenClass::SEMICOLON);
    auto comma = matcherForTokenClass(TokenClass::COMMA);
    auto lparen = matcherForTokenClass(TokenClass::L_PAREN);
    auto rparen = matcherForTokenClass(TokenClass::R_PAREN);
    auto lcurly = matcherForTokenClass(TokenClass::L_CURLY);
    auto rcurly = matcherForTokenClass(TokenClass::R_CURLY);
    auto lsquare = matcherForTokenClass(TokenClass::L_SQUARE);
    auto rsquare = matcherForTokenClass(TokenClass::R_SQUARE);
    /////////////////////////////////////////////////////
    MATCHER(paramDeclaration){
      return nullptr;
    });
    /////////////////////////////////////////////////////
    MATCHER(params){
      auto next = lparen->match(inp_view);
      if(next){
        next = paramDeclaration->match(next);

        OrkAssert(false);
        return next;
      }
      return nullptr;
    });
    /////////////////////////////////////////////////////
    MATCHER(statements){
      auto next = lparen->match(inp_view);
      if(next){
        next = paramDeclaration->match(next);

        OrkAssert(false);
        return next;
      }
      return nullptr;
    });
    /////////////////////////////////////////////////////
    _matcher_fndef = createMatcher([=](scannerlightview_constptr_t inp_view)->scannerlightview_ptr_t{
      auto seq = matcherSequence({lparen,zeroOrMore(params),rparen,lcurly,zeroOrMore(statements),rcurly});
      return seq->match(inp_view);
    });

  };

  //////////////////////////////////////////////////////////////////////

  scannerlightview_ptr_t match_fndef(const ScannerView& inp_view){
    auto slv = std::make_shared<ScannerLightView>(inp_view);
    return _matcher_fndef->match(slv);
  }

  //////////////////////////////////////////////////////////////////////

  matcher_ptr_t matcherForTokenClass(TokenClass tokclass){
    auto match_fn = [tokclass](scannerlightview_constptr_t slv) -> scannerlightview_ptr_t {
      auto slv_tokclass = slv->token_class(0);
      if(slv_tokclass==tokclass){
        auto slv_out = std::make_shared<ScannerLightView>(*slv);
        slv_out->_start++;
        return slv_out;
      }
      return nullptr;
    };
    return createMatcher(match_fn);
  }

  //////////////////////////////////////////////////////////////////////

  matcher_ptr_t matcherSequence(std::vector<matcher_ptr_t> matchers){
    auto match_fn = [matchers](scannerlightview_constptr_t slv) -> scannerlightview_ptr_t {
      auto slv_test = std::make_shared<ScannerLightView>(*slv);
      std::vector<scannerlightview_ptr_t> items;
      for(auto m:matchers){
        auto slv_match = m->match(slv_test);
        if(slv_match){
          items.push_back(slv_match);
          slv_test->_start = slv_match->_end+1;
        }
        else{
          return nullptr;
        }
      }
      OrkAssert(items.size() );
      auto slv_out = std::make_shared<ScannerLightView>(*slv);
      slv_out->_start = items.front()->_start;
      slv_out->_end = items.back()->_end;
      return slv_out;
    };
    return createMatcher(match_fn);
  }

  //////////////////////////////////////////////////////////////////////

  matcher_ptr_t oneOrMore(matcher_ptr_t matcher){
    auto match_fn = [matcher](scannerlightview_constptr_t input_slv) -> scannerlightview_ptr_t {
      std::vector<scannerlightview_ptr_t> items;
      auto slv_out = std::make_shared<ScannerLightView>(*input_slv);
      while(slv_out){
        if(slv_out){
          items.push_back(slv_out);
        }
        slv_out = matcher->match(slv_out);
      }
      if(items.size()){
        auto slv_out = std::make_shared<ScannerLightView>(*input_slv);
        slv_out->_start = items.front()->_start;
        slv_out->_end = items.back()->_end;
        return slv_out;
      }
    };
    return createMatcher(match_fn);
  }

  //////////////////////////////////////////////////////////////////////

  matcher_ptr_t zeroOrMore(matcher_ptr_t matcher){
    auto match_fn = [matcher](scannerlightview_constptr_t input_slv) -> scannerlightview_ptr_t {
      std::vector<scannerlightview_ptr_t> items;
      auto slvtest = std::make_shared<ScannerLightView>(*input_slv);
      while(slvtest){
        auto slv_matched = matcher->match(slvtest);
        if(slv_matched){
          items.push_back(slv_matched);
          slvtest->_start = slv_matched->_end+1;
        }
        else{
          slvtest = nullptr;
        }
      }

      auto slv_out = std::make_shared<ScannerLightView>(*input_slv);
      if( items.size() ){
        slv_out->_end = items.back()->_end;
      }
      return slv_out;
    };
    return createMatcher(match_fn);
  }

  //////////////////////////////////////////////////////////////////////
  
  matcher_ptr_t createMatcher(matcher_fn_t match_fn){
    auto matcher = std::make_shared<Matcher>(match_fn);
    _matchers.insert(matcher);
    return matcher;
  }

  //////////////////////////////////////////////////////////////////////

  matcher_ptr_t _matcher_fndef;
  std::unordered_set<matcher_ptr_t> _matchers;
};

//////////////////////////////////////////////////////////////

struct _ORKSL_IMPL {
  _ORKSL_IMPL(OrkSlFunctionNode* node);
  OrkslPEG _newpeg;
};

using impl_ptr_t = std::shared_ptr<_ORKSL_IMPL>;

_ORKSL_IMPL::_ORKSL_IMPL(OrkSlFunctionNode* node) {

  auto glfx_parser = node->_parser;
  auto top_node    = glfx_parser->_topNode;

  using str_set_t     = std::set<std::string>;
  using str_map_t     = std::map<std::string, std::string>;
  using struct_map_t  = std::map<std::string, structnode_ptr_t>;
  using import_vect_t = std::vector<importnode_ptr_t>;

  str_set_t valid_typenames      = top_node->_validTypeNames;
  str_set_t valid_keywords       = top_node->_keywords;
  str_set_t valid_outdecos       = top_node->_validOutputDecorators;
  struct_map_t valid_structtypes = top_node->_structTypes;
  str_map_t valid_defines        = top_node->_stddefines;
  import_vect_t imports          = top_node->_imports;

  struct RR {
    RR() {
    }
    void addRule(const char* rule, uint64_t id) {
      _id2rule[id] = rule;
    }
    const std::string& rule(TokenClass id) const {
      auto it = _id2rule.find(int(id));
      OrkAssert(it != _id2rule.end());
      return it->second;
    }
    std::map<int, std::string> _id2rule;
  };
  RR _rr;

  loadScannerRules(_rr);

  //////////////////////////////////////////////////////////////////////////////////
  // always put top first
  //////////////////////////////////////////////////////////////////////////////////

  std::string peg_rules = R"(
    ################################################
    # OrkSl TOP Level Grammar
    ################################################

    functionDefinition <- L_PAREN params? R_PAREN L_CURLY statements? R_CURLY

  )";
    

  ////////////////////////////////////////////////
  // parser rules
  ////////////////////////////////////////////////

  peg_rules += R"(

    ################################################
    # OrkSl High Level Grammar
    ################################################

    params <- paramDeclaration (COMMA paramDeclaration)*

    statements <- (statement)*

    statement <- statement_sub? SEMI_COLON

    statement_sub <- assignment_statement / functionCall / variableDeclaration / returnStatement

    functionCall <- IDENTIFIER L_PAREN arguments? R_PAREN
    returnStatement <- 'return' expression
    assignment <- IDENTIFIER EQUALS expression
    assignment_statement <- variableDeclaration EQUALS expression 
            
    expression <- IDENTIFIER / NNUMBER / functionCall / vecMatAccess

    vecMatAccess <- IDENTIFIER (L_SQUARE NNUMBER R_SQUARE / DOT component) 
    
    component <- ('x' / 'y' / 'z' / 'w' / 'r' / 'g' / 'b' / 'a')
    
    arguments <- expression (COMMA expression)*

    IDENTIFIER <- < [A-Za-z_][-A-Za-z_0-9]* > 

    dataType <- BUILTIN_TYPENAME 

    variableDeclaration <- dataType IDENTIFIER
    paramDeclaration <- dataType IDENTIFIER


    ################################################

  )";

  //////////////////////////////////////////////////////////////////////////////////

  peg_rules += R"(
    ################################################
    # OrkSl Low Level Grammar
    ################################################

    L_PAREN   <- '('
    R_PAREN   <- ')'
    L_CURLY   <- '{'
    R_CURLY   <- '}'
    L_SQUARE  <- '['
    R_SQUARE  <- ']'

    COMMA  <- ','
    DOT    <- '.'

    COLON         <- ':'
    DOUBLE_COLON  <- '::'
    SEMI_COLON    <- ';'
    QUESTION_MARK <- '?'

    EQUALS        <- '='
    STAR_EQUALS   <- '*='
    PLUS_EQUALS   <- '+='
    ASSIGNMENT_OP <- (EQUALS/STAR_EQUALS/PLUS_EQUALS)

    EQUAL_TO               <- '=='
    NOT_EQUAL_TO           <- '!='
    LESS_THAN              <- '<'
    GREATER_THAN           <- '>'
    LESS_THAN_EQUAL_TO     <- '<'
    GREATER_THAN_EQUAL_TO  <- '>'

    PLUS   <- '+'
    MINUS  <- '-'
    STAR   <- '*'
    SLASH  <- '/'
    CARET  <- '^'
    EXCLAMATION  <- '^'
    TILDE  <- '~'

    AMPERSAND    <- '&'
    PIPE         <- '|'
    LOGICAL_AND  <- '&&'
    LOGICAL_OR   <- '||'
    LEFT_SHIFT   <- '<<'
    RIGHT_SHIFT  <- '>>'

    PLUS_PLUS    <- '++'
    MINUS_MINUS  <- '--'

    INTEGER          <- (HEX_INTEGER/DEC_INTEGER)
    FLOAT            <- <MINUS? [0-9]+ '.' [0-9]*>
    DEC_INTEGER      <- <MINUS? [0-9]+ 'u'?>
    HEX_INTEGER      <- < ('x'/'0x') [0-9a-fA-F]+ 'u'? >
    NUMBER           <- (FLOAT/INTEGER)
    NNUMBER <- [0-9]+ (DOT [0-9]+)? 

    WHITESPACE          <- [ \t\r\n]*
    %whitespace         <- WHITESPACE

    TYPENAME <- (BUILTIN_TYPENAME)

)";

  ////////////////////////////////////////////////
  peg_rules += "    ################################################\n\n";
  peg_rules += FormatString("    KEYWORD <- (");
  size_t num_keywords = valid_keywords.size();
  int ik = 0;
  for( auto item : valid_keywords  ){
    bool is_last = (ik==(num_keywords-1));
    auto format_str = is_last ? "'%s'" : "'%s'/";
    peg_rules += FormatString(format_str, item.c_str() );
    ik++;
  }
  peg_rules += FormatString(")\n\n");

  ////////////////////////////////////////////////
  peg_rules += "    ################################################\n\n";
  peg_rules += FormatString("    BUILTIN_TYPENAME <- (");
  size_t num_typenames = valid_typenames.size();
  int it = 0;
  for( auto item : valid_typenames ){
    bool is_last = (it==(num_typenames-1));
    auto format_str = is_last ? "'%s'" : "'%s'/";
    peg_rules += FormatString(format_str, item.c_str() );
    it++;
  }
  peg_rules += FormatString(")\n\n");

  ////////////////////////////////////////////////
}

/////////////////////////////////////////////////////////////////////////////////////////////////

svar16_t OrkSlFunctionNode::_getimpl(OrkSlFunctionNode* node) {
  static auto _gint = std::make_shared<_ORKSL_IMPL>(node);
  return _gint;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

OrkSlFunctionNode::OrkSlFunctionNode(parser_rawptr_t parser)
    : AstNode(parser) {
}

/////////////////////////////////////////////////////////////////////////////////////////////////

int OrkSlFunctionNode::parse(const ScannerView& view) {

  auto internals = _getimpl(this).get<impl_ptr_t>();

  int i = 0;
  view.dump("OrkSlFunctionNode::start");
  auto open_tok = view.token(i);
  OrkAssert(open_tok->text == "(");
  i++;

  internals->_newpeg.match_fndef(view);
  OrkAssert(false);

  return 0;
}
void OrkSlFunctionNode::emit(shaderbuilder::BackEnd& backend) const {
  OrkAssert(false);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2::glslfx::parser
/////////////////////////////////////////////////////////////////////////////////////////////////

#endif