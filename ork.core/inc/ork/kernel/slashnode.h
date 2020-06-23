////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2020, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#pragma once

#include <ork/orkstl.h>

///////////////////////////////////////////////////////////////////////////////

namespace ork {

class SlashTree;
class SlashNode;
using slashnode_ptr_t      = std::shared_ptr<SlashNode>;
using slashtree_ptr_t      = std::shared_ptr<SlashTree>;
using slashnode_constptr_t = std::shared_ptr<const SlashNode>;
using slashtree_constptr_t = std::shared_ptr<const SlashTree>;

void breakup_slash_path(std::string str, orkvector<std::string>& outvec);

/// Returns the first occurence of a character within a string.
///
/// @param str The string to search
/// @param cch The character to search for
/// @param start The index at which to start the search
/// @return The index of cch within str (starting at start). If cch is not found, return std::string::npos.
std::string::size_type str_cue_to_char(const std::string& str, char cch, int start);

///////////////////////////////////////////////////////////////////////////////

class SlashTree;

class SlashNode {
  friend class SlashTree;

  using children_t = orkmap<std::string, slashnode_ptr_t>;

  std::string name;
  SlashNode* parent;
  children_t children_map;
  void* data;

public: //
  void add_child(slashnode_ptr_t child);
  std::string getfullpath(void) const;
  SlashNode();
  ~SlashNode();

  int GetNumChildren() const {
    return int(children_map.size());
  }
  const children_t& GetChildren() const {
    return children_map;
  }
  const std::string& GetNodeName() const {
    return name;
  }
  bool IsLeaf(void) const {
    return (0 == GetNumChildren());
  }
  void dump(void);

  void SetData(void* pdata) {
    data = pdata;
  }
  const void* GetData(void) const {
    return data;
  }

  void GetPath(orkvector<const SlashNode*>& pth) const;
};

///////////////////////////////////////////////////////////////////////////////

class SlashTree {

  slashnode_ptr_t _root;

public: //
  slashnode_ptr_t add_node(const char* instr, void* ndata);
  void remove_node(SlashNode* pnode);
  slashnode_ptr_t find_node(const std::string& instr) const;
  SlashTree();
  void Clear(void);
  void dump(void) const;

  slashnode_constptr_t root(void) const {
    return _root;
  }
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ork
