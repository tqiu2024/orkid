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

match_results_t AdditiveExpression::match(FnParseContext ctx) {
  match_results_t rval;
  rval            = std::make_shared<match_t>(ctx);
  bool done       = false;
  bool danglingop = false;
  while (not done) {
    if (auto mso = MultiplicativeExpression::match(ctx)) {
      if (not rval) {
        rval = std::make_shared<match_t>(ctx);
      }
      rval       = rval + mso;
      danglingop = false;
      ctx        = mso->consume();
      return rval;
    }
    if (auto mlt = AddOp::match(ctx)) {
      rval       = rval + mlt;
      danglingop = true;
      ctx        = mlt->consume();
    } else if (auto mrt = SubOp::match(ctx)) {
      rval       = rval + mrt;
      danglingop = true;
      ctx        = mrt->consume();
    } else {
      done = true;
    }
  }
  assert(false == danglingop);
  return rval;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t AndExpression::match(FnParseContext ctx) {
  match_results_t rval;
  rval             = std::make_shared<match_t>(ctx);
  bool done        = false;
  bool danglingand = false;
  while (not done) {
    if (auto eeo = EqualityExpression::match(ctx)) {
      if (not rval) {
        rval = std::make_shared<match_t>(ctx);
      }
      ctx         = eeo->consume();
      rval        = rval + eeo;
      danglingand = false;
    }
    if (auto mao = AndOp::match(ctx)) {
      ctx         = mao->consume();
      rval        = rval + mao;
      danglingand = true;
    } else {
      done = true;
    }
  }
  assert(false == danglingand);
  return rval;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t ArgumentExpressionList::match(FnParseContext ctx) {
  const auto ctxbase = ctx;
  match_results_t rval;
  bool done = false;
  while (not done) {
    if (ctx.tokenValue(0) == "1.111") {
     // printf("yo\n");
    }
    if (auto m = ExpressionNode::match(ctx)) {
      if (not rval) {
        rval = std::make_shared<match_t>(ctx);
      }
      rval = rval + m;
      ctx  = rval->consume();
      if (auto m2 = CloseParen::match(ctx))
        done = true;
      else if (auto m3 = CommaOp::match(ctx)) {
        rval = rval + m3;
        ctx  = rval->consume();
      }
    } else {
      done = true;
    }
  }
  return rval;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t ConditionalExpression::match(FnParseContext ctx) {
  match_results_t rval;
  if (auto mte = TernaryExpression::match(ctx)) {
    rval = mte;
  } else if (auto mtl = LogicalOrExpression::match(ctx)) {
    rval = mtl;
  }
  return rval;
}

// parsed_t ConditionalExpression::parse(const match_t& match) {
// parsed_t rval;

// return rval;
//}
// void Expression::emit(shaderbuilder::BackEnd& backend) const {
// assert(false);
//}

match_results_t CastExpression::match(FnParseContext ctx) {
  match_results_t rval;
  if (auto m = UnaryExpression::match(ctx)) {
    rval = m;
  } else if (auto m = OpenParen::match(ctx)) {
    auto c2 = m->consume();
    if (auto m2 = TypeName::match(c2)) {
      auto c3 = (m + m2)->consume();
      if (auto m3 = CloseParen::match(c3)) {
        rval = std::make_shared<match_t>((m + m2 + m3)->consume());
      }
    }
  }
  return rval;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t ExclusiveOrExpression::match(FnParseContext ctx) {
  match_results_t rval;
  rval             = std::make_shared<match_t>(ctx);
  bool done        = false;
  bool danglingxor = false;
  while (not done) {
    if (auto meo = AndExpression::match(ctx)) {
      if (not rval) {
        rval = std::make_shared<match_t>(ctx);
      }
      ctx         = meo->consume();
      rval        = rval + meo;
      danglingxor = false;
      return rval;
    }
    if (auto xoo = XorOp::match(ctx)) {
      ctx         = xoo->consume();
      rval        = rval + xoo;
      danglingxor = true;
    } else {
      done = true;
    }
  }
  assert(false == danglingxor);
  return rval;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t EqualityExpression::match(FnParseContext ctx) {
  match_results_t rval;
  rval            = std::make_shared<match_t>(ctx);
  bool done       = false;
  bool danglingop = false;
  while (not done) {
    if (auto mro = RelationalExpression::match(ctx)) {
      if (not rval) {
        rval = std::make_shared<match_t>(ctx);
      }
      ctx        = mro->consume();
      rval       = rval + mro;
      danglingop = false;
      return rval;
    }
    if (auto meo = EqOp::match(ctx)) {
      ctx        = meo->consume();
      rval       = rval + meo;
      danglingop = true;
    } else if (auto mno = NeqOp::match(ctx)) {
      ctx        = mno->consume();
      rval       = rval + mno;
      danglingop = true;
    } else {
      done = true;
    }
  }
  assert(false == danglingop);
  return rval;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t Expression::match(FnParseContext ctx) {
  match_results_t rval;
  size_t count = 0;
  size_t start = -1;
  bool done    = false;
  bool match   = true;
  while (not done) {
    auto mva = ExpressionNode::match(ctx);
    mva.dump("enode");
    if (mva) {
      // todo fix left recursive
      // return mva;
    }
    auto try_tok = ctx.tokenValue(0);
    if (try_tok == ",") {
      ctx._startIndex++;
      count++;
    } else if (try_tok == ";") {
      done = true;
    } else {
      done  = true;
      match = false;
    }
  }
  if (match) {
    rval           = std::make_shared<match_t>(ctx);
    rval->_count   = count;
    rval->_start   = start;
    rval->_matched = true;
  }
  return rval;
}

// parsed_t Expression::parse(const match_t& match) {
// parsed_t rval;
// assert(false);
// return rval;
//}
// void Expression::emit(shaderbuilder::BackEnd& backend) const {
// assert(false);
//}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t ExpressionNode::match(FnParseContext ctx) {
  match_results_t rval;
  rval              = std::make_shared<match_t>(ctx);
  bool done         = false;
  size_t numparen   = 0;
  bool has_typename = false;
  bool has_ref      = false;
  bool has_idp      = false;
  // ctx._view->dump("ExpressionNode");
  while (not done) {
    // ctx._view->dump("ExpressionNode::A");
    if (auto mvo = OpenParen::match(ctx)) {
      // ctx._view->dump("ExpressionNode::(");
      rval = rval + mvo;
      ctx  = rval->consume();
      numparen++;
    } else if (auto mvo = CloseParen::match(ctx)) {
      // ctx._view->dump("ExpressionNode::)");
      numparen--;
      rval = rval + mvo;
      ctx  = rval->consume();
    } else if (auto mvq = TypeName::match(ctx)) {
      // ctx._view->dump("ExpressionNode::typename");
      has_typename = true;
      rval         = rval + mvq;
      ctx          = mvq->consume();
      if (auto mvd = DotOp::match(ctx)) {
        rval = rval + mvd;
        ctx  = rval->consume();
      }
    } else if (auto mvq = Reference::match(ctx)) {
      // ctx._view->dump("ExpressionNode::reference");
      rval    = rval + mvq;
      has_ref = true;
      ctx     = rval->consume();
      if (auto mvd = DotOp::match(ctx)) {
        rval = rval + mvq;
        ctx  = rval->consume();
      }
    } else if (auto mvc = Constant::match(ctx)) {
      // ctx._view->dump("ExpressionNode::constant");
      rval = rval + mvc;
      ctx  = rval->consume();
    } else if (auto mvo = MathOp::match(ctx)) {
      // ctx._view->dump("ExpressionNode::uop");
      rval = rval + mvo;
      ctx  = rval->consume();
    } else if (auto mvo = MutatingAssignmentOperator::match(ctx)) {
      // ctx._view->dump("ExpressionNode::mao");
      rval = rval + mvo;
      ctx  = rval->consume();
    } else if (auto mve = CommaOp::match(ctx)) {
      // ctx._view->dump("CommaOp::subexp");
      rval = rval + mve;
      ctx  = rval->consume();
    } else {
      if (numparen > 0) {
        printf("numparen<%zd>\n", numparen);
        ctx.dump("ExpressionNode::???");
        rval.dump("rven");
        OrkAssert(false);
      } else
        done = true;
    }
  }
  assert(numparen == 0);
  return rval;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t InclusiveOrExpression::match(FnParseContext ctx) {
  match_results_t rval;
  rval            = std::make_shared<match_t>(ctx);
  bool done       = false;
  bool danglingor = false;
  while (not done) {
    if (auto meo = ExclusiveOrExpression::match(ctx)) {
      if (not rval) {
        rval = std::make_shared<match_t>(ctx);
      }
      ctx        = meo->consume();
      rval       = rval + meo;
      danglingor = false;
      return rval;
    }
    if (auto moo = OrOp::match(ctx)) {
      ctx        = moo->consume();
      rval       = rval + moo;
      danglingor = true;
    } else {
      done = true;
    }
  }
  assert(false == danglingor);
  return rval;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t LogicalOrExpression::match(FnParseContext ctx) {
  match_results_t rval;
  rval            = std::make_shared<match_t>(ctx);
  bool done       = false;
  bool danglingor = false;
  while (not done) {
    if (auto mla = LogicalAndExpression::match(ctx)) {
      ctx        = mla->consume();
      rval       = rval + mla;
      danglingor = false;
      return rval;
    }
    if (auto moo = OrOrOp::match(ctx)) {
      ctx        = moo->consume();
      rval       = rval + moo;
      danglingor = true;
    } else {
      done = true;
    }
  }
  assert(false == danglingor);
  return rval;
}

// parsed_t LogicalOrExpression::parse(const match_t& match) {
// parsed_t rval;
// assert(false);
// return rval;
//}
// void Expression::emit(shaderbuilder::BackEnd& backend) const {
// assert(false);
//}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t LogicalAndExpression::match(FnParseContext ctx) {
  match_results_t rval;
  bool done        = false;
  bool danglingand = false;
  while (not done) {
    if (auto mio = InclusiveOrExpression::match(ctx)) {
      if (not rval) {
        rval = std::make_shared<match_t>(ctx);
      }
      ctx         = mio->consume();
      rval        = rval + mio;
      danglingand = false;
      return rval;
    }
    if (auto mao = AndAndOp::match(ctx)) {
      ctx         = mao->consume();
      rval        = rval + mao;
      danglingand = true;
    } else {
      done = true;
    }
  }
  assert(false == danglingand);
  return rval;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t MultiplicativeExpression::match(FnParseContext ctx) {
  match_results_t rval;
  rval            = std::make_shared<match_t>(ctx);
  bool done       = false;
  bool danglingop = false;
  while (not done) {
    if (not rval) {
      rval = std::make_shared<match_t>(ctx);
    }
    if (auto mso = CastExpression::match(ctx)) {
      rval       = rval + mso;
      ctx        = mso->consume();
      danglingop = false;
      return rval;
    }
    if (auto mlt = MulOp::match(ctx)) {
      rval       = rval + mlt;
      ctx        = mlt->consume();
      danglingop = true;
    } else if (auto mrt = DivOp::match(ctx)) {
      rval       = rval + mrt;
      ctx        = mrt->consume();
      danglingop = true;
    } else if (auto mrt = ModOp::match(ctx)) {
      rval       = rval + mrt;
      ctx        = mrt->consume();
      danglingop = true;
    } else {
      done = true;
    }
  }
  assert(false == danglingop);
  return rval;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t PostFixExpression::match(FnParseContext ctx, int level) {
  /////////////////////////////////////////////////
  // limit left recursion
  /////////////////////////////////////////////////
  if(level>8){
    return match_results_t(); // no match
  }
  /////////////////////////////////////////////////
  if (auto m0 = PrimaryExpression::match(ctx)) {
    return m0; // primary_expression
  } else if (auto m0 = PostFixExpression::match(ctx,level+1)) {
    auto c1 = m0->consume();
    /////////////////////////////////////////////////
    if (auto m1 = OpenSquare::match(c1)) {
      auto c2 = m1->consume();
      if (auto m2 = Expression::match(c2)) {
        auto c3 = m2->consume();
        if (auto m3 = CloseSquare::match(c3)) {
          return (m0 + m1 + m2 + m3); // postfix_expression [ expression ]
        }
      }
    }
    /////////////////////////////////////////////////
    else if (auto m1 = OpenParen::match(c1)) {
      auto c2 = m1->consume();
      if (auto m2 = CloseParen::match(c2)) {
        return (m0 + m1 + m2); // postfix_expression ()
      } else if (auto m2 = ArgumentExpressionList::match(c2)) {
        auto c3 = m2->consume();
        if (auto m3 = CloseParen::match(c3)) {
          return (m0 + m1 + m2 + m3); // postfix_expression ( argument_expression_list )
        }
      }
    }
    /////////////////////////////////////////////////
    else if (auto m1 = DotOp::match(c1)) {
      auto c2 = m1->consume();
      if (auto m2 = Identifier::match(c2)) {
        return (m0 + m1 + m2); // postfix_expression . identifier
      }
    }
    /////////////////////////////////////////////////
    else if (auto m1 = IncOp::match(ctx)) {
      return (m0+m1); // postfix_expression inc_op
    }
    /////////////////////////////////////////////////
    else if (auto m = DecOp::match(ctx)) {
      return (m0+m1); // postfix_expression dec_op
    }
  } // else if (auto m0 = PostFixExpression::match(ctx)) {
  return match_results_t(); // no match
}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t PrimaryExpression::match(FnParseContext ctx) {
  if (auto m0 = Identifier::match(ctx))
    return m0; // identifier
  else if (auto m0 = Constant::match(ctx))
    return m0; // constant
  else if (auto m0 = StringLiteral::match(ctx))
    return m0; // string_literal
  else if (auto m0 = OpenParen::match(ctx)) {
    auto c1 = m0->consume();
    if (auto m1 = Expression::match(c1)) {
      auto c2 = (m0 + m1)->consume();
      if (auto m2 = CloseParen::match(c2)) {
        return (m0 + m1 + m2);  // ( expression )
      }
    }
  }
  return match_results_t(); // no match
}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t RelationalExpression::match(FnParseContext ctx) {
  match_results_t rval;
  rval            = std::make_shared<match_t>(ctx);
  bool done       = false;
  bool danglingop = false;
  while (not done) {
    if (not rval) {
      rval = std::make_shared<match_t>(ctx);
    }
    if (auto mso = ShiftExpression::match(ctx)) {
      ctx        = mso->consume();
      rval       = rval + mso;
      danglingop = false;
      return rval;
    }
    if (auto mlt = LtOp::match(ctx)) {
      ctx        = mlt->consume();
      rval       = rval + mlt;
      danglingop = true;
    } else if (auto mlte = LtEqOp::match(ctx)) {
      ctx        = mlte->consume();
      rval       = rval + mlte;
      danglingop = true;
    } else if (auto mgt = GtOp::match(ctx)) {
      ctx        = mgt->consume();
      rval       = rval + mgt;
      danglingop = true;
    } else if (auto mgte = GtEqOp::match(ctx)) {
      ctx        = mgte->consume();
      rval       = rval + mgte;
      danglingop = true;
    } else {
      done = true;
    }
  }
  assert(false == danglingop);
  return rval;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t ShiftExpression::match(FnParseContext ctx) {
  match_results_t rval;
  rval            = std::make_shared<match_t>(ctx);
  bool done       = false;
  bool danglingop = false;
  while (not done) {
    if (auto mso = AdditiveExpression::match(ctx)) {
      if (not rval) {
        rval = std::make_shared<match_t>(ctx);
      }
      ctx        = mso->consume();
      rval       = rval + mso;
      danglingop = false;
      return rval;
    }
    if (auto mlt = LeftOp::match(ctx)) {
      ctx        = mlt->consume();
      rval       = rval + mlt;
      danglingop = true;
    } else if (auto mrt = RightOp::match(ctx)) {
      ctx        = mrt->consume();
      rval       = rval + mrt;
      danglingop = true;
    } else {
      done = true;
    }
  }
  assert(false == danglingop);
  return rval;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t TernaryExpression::match(FnParseContext ctx) {
  match_results_t rval;
  rval = std::make_shared<match_t>(ctx);
  if (auto mvl = LogicalOrExpression::match(ctx)) {
    if (ctx.tokenValue(mvl->end()) == "?") {
      auto ctx2 = mvl->consume();
      ctx2._startIndex++; // consume ?
      if (auto mve = Expression::match(ctx2)) {
        if (ctx.tokenValue(mve->end()) == ":") {
          auto ctx3 = mve->consume();
          ctx3._startIndex++; // consume :
          if (auto mvx = ConditionalExpression::match(ctx3)) {
            rval = mvx;
            assert(false);
          }
        }
      }
    }
  }
  return rval;
}

// parsed_t TernaryExpression::parse(const match_t& match) {
// parsed_t rval;

// return rval;
//}
// void Expression::emit(shaderbuilder::BackEnd& backend) const {
// assert(false);
//}

/////////////////////////////////////////////////////////////////////////////////////////////////

match_results_t UnaryExpression::match(FnParseContext ctx) {
  if (auto m = PostFixExpression::match(ctx)) {
    return m;
  } else if (auto m = IncOp::match(ctx)) {
    auto c2 = m->consume();
    if (auto m2 = UnaryExpression::match(c2)) {
      return (m + m2);
    }
  } else if (auto m = DecOp::match(ctx)) {
    auto c2 = m->consume();
    if (auto m2 = UnaryExpression::match(c2)) {
      return (m + m2);
    }
  } else if (auto m = MathOp::match(ctx)) {
    auto c2 = m->consume();
    if (auto m2 = CastExpression::match(c2)) {
      return (m + m2);
    }
  } else if (auto m = SizeofOp::match(ctx)) {
    auto c2 = m->consume();
    if (auto m2 = UnaryExpression::match(c2)) {
      return (m + m2);
    } else if (auto m2 = OpenParen::match(c2)) {
      auto c3 = (m + m2)->consume();
      if (auto m3 = TypeName::match(c3)) {
        auto c4 = (m + m2 + m3)->consume();
        if (auto m4 = CloseParen::match(c4)) {
          return m + m2 + m3 + m4;
        }
      }
    }
  }
  return match_results_t(); // no match
}

// UnaryExpression::parsed_t UnaryExpression::parse(const match_t& match) {
// parsed_t rval;
// assert(false);
// return rval;
//}
// void Expression::emit(shaderbuilder::BackEnd& backend) const {
// assert(false);
//}

/////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2::glslfx::parser
/////////////////////////////////////////////////////////////////////////////////////////////////

#endif 
