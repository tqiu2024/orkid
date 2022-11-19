////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
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
#include <peglib.h>

/////////////////////////////////////////////////////////////////////////////////////////////////
namespace ork::lev2::glslfx::parser {
/////////////////////////////////////////////////////////////////////////////////////////////////

using peg_parser_ptr_t = std::shared_ptr<peg::parser>;

struct _ORKSL_IMPL {
  _ORKSL_IMPL(OrkSlFunctionNode* node);
  peg_parser_ptr_t _peg_parser;
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
    void addRule(const char* rule, int id) {
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

  ////////////////////////////////////////////////
  // parser rules
  ////////////////////////////////////////////////

  std::string peg_rules = R"(
    # OrkSl Grammar

    top  <- argument_decl_list function_body 

    ################################################
    # low level constructs
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

    WHITESPACE          <- [ \t\n]*
    %whitespace         <- WHITESPACE

    TYPENAME <- (BUILTIN_TYPENAME)
    IDENTIFIER <- < [A-Za-z_][-A-Za-z_0-9]* >

    ################################################
    # language constructs
    ################################################

    function_body        <- L_CURLY statement_list* R_CURLY

    statement_list       <- statement+

    statement            <- (return_statement/for_statement/assignment_statement2/assignment_statement) SEMI_COLON+

    return_statement     <- 'return' expression*
    for_statement        <- 'for' L_PAREN assignment_statement2 SEMI_COLON expression SEMI_COLON expression? R_PAREN L_CURLY statement_list* R_CURLY
    assignment_statement2 <- TYPENAME IDENTIFIER EQUALS expression
    assignment_statement <- IDENTIFIER ASSIGNMENT_OP expression

    binary_math_op       <- (PLUS/MINUS/STAR/SLASH/AMPERSAND/PIPE)
    comparison_op        <- (EQUAL_TO/NOT_EQUAL_TO/LESS_THAN/GREATER_THAN/LESS_THAN_EQUAL_TO/GREATER_THAN_EQUAL_TO)
    shift_op             <- (LEFT_SHIFT/RIGHT_SHIFT)
    inc_dec_op           <- (PLUS_PLUS/MINUS_MINUS)
    operator             <- (inc_dec_op/shift_op/binary_math_op/comparison_op/DOT/COMMA/EQUALS)

    expression           <- (primary_expression3/primary_expression2/primary_expression)

    primary_expression2  <- L_PAREN expression+ R_PAREN

    primary_expression3  <- primary_expression operator primary_expression
                          / primary_expression2 operator primary_expression   
                          / primary_expression operator primary_expression2   
                          / primary_expression2 operator primary_expression2   

    primary_expression   <- (TYPENAME/KEYWORD/IDENTIFIER/operator/NUMBER)+

    argument_decl_list  <- L_PAREN arg_items R_PAREN
    arg_pair            <- TYPENAME IDENTIFIER
    comma_arg_pair      <- COMMA TYPENAME IDENTIFIER 
    arg_items           <- arg_pair comma_arg_pair*

    ################################################

  )";

  ////////////////////////////////////////////////

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
  // parser initialization
  ////////////////////////////////////////////////

  printf( "peg_rules: %s\n", peg_rules.c_str() );

  _peg_parser = std::make_shared<peg::parser>();

  _peg_parser->set_logger([](size_t line,               //
                             size_t col,                //
                             const std::string& msg,    //
                             const std::string& rule) { //
    std::cerr << line << ":" << col << ": " << rule << " : " << msg << "\n";
  });

  auto& parser = *_peg_parser;

  parser.load_grammar(peg_rules);

  // validate rules

  if( not static_cast<bool>(parser) ){

    auto as_lines = SplitString(peg_rules,'\n');
    int iline = 1;
    for( auto l: as_lines ){
      printf( "%03d: %s\n", iline, l.c_str() );
      iline++;
    }
    OrkAssert(false);

  }

  ///////////////////////////////////////////////////////////


  auto impl_default_handler = [&parser](std::string key){
    parser[key.c_str()] = [key](const peg::SemanticValues& vs) {
      auto tok = vs.token_to_string(0);
      printf("%s: %s\n", key.c_str(), tok.c_str());
    };
  };

  ///////////////////////////////////////////////////////////
  // keyword / typename / identifier semantic actions
  ///////////////////////////////////////////////////////////

  impl_default_handler("KEYWORD");
  impl_default_handler("TYPENAME");
  impl_default_handler("IDENTIFIER");

  /*parser["KEYWORD"].predicate = [valid_keywords](const peg::SemanticValues& vs, const std::any& dt, std::string& msg) {
    auto tok = vs.token_to_string(0);
    auto it = valid_keywords.find(tok);
    return (it!=valid_keywords.end());
  };*/


  //parser["TYPENAME"].predicate = [valid_typenames](const peg::SemanticValues& vs, const std::any& /*dt*/, std::string& msg) {
    //auto tok = vs.token_to_string(0);
    //auto it = valid_typenames.find(tok);
    //return (it!=valid_typenames.end());
  //};


  parser["IDENTIFIER"].predicate = [valid_typenames,valid_keywords](const peg::SemanticValues& vs, const std::any& /*dt*/, std::string& msg) {
    auto tok = vs.token_to_string(0);
    auto itt = valid_typenames.find(tok);
    auto itk = valid_keywords.find(tok);
    return (itt==valid_typenames.end()) and (itk==valid_keywords.end());
  };

  ///////////////////////////////////////////////////////////
  // hierarchy token semantic actions
  ///////////////////////////////////////////////////////////

  impl_default_handler("L_PAREN");
  impl_default_handler("R_PAREN");
  impl_default_handler("L_CURLY");
  impl_default_handler("R_CURLY");
  impl_default_handler("L_SQUARE");
  impl_default_handler("R_SQUARE");
  impl_default_handler("DOT");
  impl_default_handler("COMMA");

  impl_default_handler("NUMBER");
  //impl_default_handler("INTEGER");
  //impl_default_handler("FLOAT");
  
  ///////////////////////////////////////////////////////////
  // language construct semantic actions
  ///////////////////////////////////////////////////////////

  impl_default_handler("top");
  impl_default_handler("argument_decl_list");
  impl_default_handler("arg_pair_comma");
  impl_default_handler("arg_items");
  impl_default_handler("function_body");
  impl_default_handler("statement_list");
  //impl_default_handler("statement");
  //impl_default_handler("empty_statement");
  impl_default_handler("return_statement");
  impl_default_handler("for_statement");
  impl_default_handler("assignment_statement");
  impl_default_handler("assignment_statement2");
  impl_default_handler("expression");
  impl_default_handler("primary_expression");
  impl_default_handler("primary_expression2");
  impl_default_handler("operator");


  /*_grules.push("CONSTANT", //
    "  FLOAT " //
    "| UINT" //
    "| XINT" //
  );*/

  /*_additive_expression = _grules.push("additive_expression", //
    "  multiplicative_expression " //
    "| additive_expression PLUS multiplicative_expression" //
    "| additive_expression MINUS multiplicative_expression" //
  );

  _and_expression = _grules.push("and_expression", //
    "  equality_expression " //
    "| and_expression AMPERSAND equality_expression" //
  );

  _argument_expression_list = _grules.push("argument_expression_list", //
    "  assignment_expression " //
    "| argument_expression_list COMMA assignment_expression" //
  );

  _assignment_expression = _grules.push("assignment_expression", //
    "  conditional_expression " //
    "| unary_expression assignment_operator assignment_expression " //
  );

  _assignment_operator = _grules.push("assignment_operator", //
    "  EQUALS" //
    //"| EQUALS" //
  );

  _cast_expression = _grules.push("cast_expression", //
    "  unary_expression " //
    "| L_PAREN TYPENAME R_PAREN " //
  );


  _conditional_expression = _grules.push("conditional_expression", //
    "  logical_or_expression " //
    "| logical_or_expression ternary_expression " //
  );

  _equality_expression = _grules.push("equality_expression", //
    "  relational_expression " //
    "| equality_expression EQUAL_TO relational_expression" //
    "| equality_expression NOT_EQUAL_TO relational_expression" //
  );

  _exclusive_or_expression = _grules.push("exclusive_or_expression", //
    "  and_expression " //
    "| exclusive_or_expression CARET and_expression " //
  );

  _expression = _grules.push("expression", //
    "  assignment_expression " //
    "| expression COMMA assignment_expression" //
  );

  _inclusive_or_expression = _grules.push("inclusive_or_expression", //
    "  logical_or_expression " //
    "| inclusive_or_expression PIPE exclusive_or_expression " //
  );

  _logical_and_expression = _grules.push("logical_and_expression", //
    "  inclusive_or_expression " //
    "| logical_and_expression LOGICAL_AND inclusive_or_expression " //
  );

  _logical_or_expression = _grules.push("logical_or_expression", //
    "  logical_and_expression " //
    "| logical_or_expression LOGICAL_OR logical_and_expression " //
  );

  _multiplicative_expression = _grules.push("multiplicative_expression", //
    "  cast_expression " //
    "| multiplicative_expression STAR cast_expression " // *
    "| multiplicative_expression SLASH cast_expression " // /
    "| multiplicative_expression PERCENT cast_expression " // %
  );

  _postfix_expression = _grules.push("postfix_expression", //
    "  primary_expression " //
    "| postfix_expression L_SQUARE expression R_SQUARE " // *
    "| postfix_expression L_PAREN R_PAREN " // *
    "| postfix_expression L_PAREN argument_expression_list R_PAREN " // *
    "| postfix_expression DOT IDENTIFIER " // *
    "| postfix_expression INCREMENT " // *
    "| postfix_expression DECREMENT " // *
  );

  _primary_expression = _grules.push("primary_expression", //
    "| IDENTIFIER " // *
    "| CONSTANT " // *
    "| STRING_LITERAL " // *
    "| L_PAREN expression R_PAREN " // *
  );

  _relational_expression = _grules.push("relational_expression", //
    "  shift_expression " //
    "| relational_expression LESS_THAN shift_expression " //
    "| relational_expression LESS_THAN_EQ shift_expression " //
    "| relational_expression GREATER_THAN shift_expression " //
    "| relational_expression GREATER_THAN_EQ shift_expression " //
  );

  _shift_expression = _grules.push("shift_expression", //
    "  additive_expression " //
    "| shift_expression L_SHIFT additive_expression " //
    "| shift_expression R_SHIFT additive_expression " //
  );

  _ternary_expression = _grules.push("ternary_expression", //
    "QUESTION_MARK expression COLON conditional_expression" //
  );

  _unary_expression = _grules.push("unary_expression", //
    "  postfix_expression " //
    "| INCREMENT unary_expression " //
    "| DECREMENT unary_expression " //
    "| unary_operator cast_expression " //
  );

  _unary_operator = _grules.push("unary_operator", //
    "  AMPERSAND " //
    "| STAR " //
    "| PLUS " //
    "| MINUS " //
    "| EXCLAMATION " //
    //"| TILDE " // ~
  );*/

  ////////////////////////////////////////////////
}

svar16_t OrkSlFunctionNode::_getimpl(OrkSlFunctionNode* node) {
  static auto _gint = std::make_shared<_ORKSL_IMPL>(node);
  return _gint;
}

OrkSlFunctionNode::OrkSlFunctionNode(parser_rawptr_t parser)
    : AstNode(parser) {
}

int OrkSlFunctionNode::parse(const ScannerView& view) {

  auto internals = _getimpl(this).get<impl_ptr_t>();

  int i = 0;
  view.dump("OrkSlFunctionNode::start");
  auto open_tok = view.token(i);
  OrkAssert(open_tok->text == "(");
  i++;

  try {

    std::string input = view.asString();

    printf("input<%s>\n", input.c_str());

    auto str_start = input.c_str();
    auto str_end   = str_start + 14; // input.size();

    printf("input.st<%c>\n", *str_start);
    printf("input.en<%c>\n", *str_end);

    bool ret = internals->_peg_parser->parse(input);

    OrkAssert(ret==true);

  } catch (const std::exception& e) {
    std::cout << e.what() << '\n';
    OrkAssert(false);
  }
  return 0;
}
void OrkSlFunctionNode::emit(shaderbuilder::BackEnd& backend) const {
  OrkAssert(false);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2::glslfx::parser
/////////////////////////////////////////////////////////////////////////////////////////////////
