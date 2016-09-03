/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-2008 Hex-Rays
 *      ALL RIGHTS RESERVED.
 *
 *
 *      Graph drawing support
 *
 */

#ifndef __GDLDRAW_HPP
#define __GDLDRAW_HPP

#include <area.hpp>
#include <algorithm>
#include <vector>
#include <deque>
#include <map>
#include <set>

#pragma pack(push, 4)

//-------------------------------------------------------------------------
// forward declarations:
class node_iterator;
class qflow_chart_t;
class gdl_graph_t;

// flow chart block types
enum fc_block_type_t
{
  fcb_normal,    // normal block
  fcb_indjump,   // block ends with indirect jump
  fcb_ret,       // return block
  fcb_cndret,    // conditional return block
  fcb_noret,     // noreturn block
  fcb_enoret,    // external noreturn block (does not belong to the function)
  fcb_extern,    // external normal block
  fcb_error,     // block passes execution past the function end
};

#ifndef SWIG
#define DECLARE_HELPER(decl)                                        \
decl node_iterator &ida_export node_iterator_goup(node_iterator *); \
decl void ida_export create_qflow_chart(qflow_chart_t &);           \
decl bool ida_export append_to_flowchart(qflow_chart_t &, ea_t, ea_t); \
decl fc_block_type_t ida_export fc_calc_block_type(const qflow_chart_t &, size_t); \
decl bool ida_export create_multirange_qflow_chart(qflow_chart_t &, const areavec_t &);
#else
#define DECLARE_HELPER(decl)
#endif // SWIG

DECLARE_HELPER(idaman)

//-------------------------------------------------------------------------
class intseq_t : public qvector<int>
{
public:
  int  idaapi index(int value) const
  {
    for ( int i=0; i < size(); i++ )
      if ( (*this)[i] == value )
        return i;
    return -1;
  }
  bool idaapi contains(int value) const { return index(value) != -1; }
  void idaapi add(int value) { push_back(value); }
  bool idaapi add_unique(int value)
  {
    if ( contains(value) )
      return false;
    add(value);
    return true;
  }
  void idaapi add_unique(const intseq_t &s);
  int  idaapi get(void) { int v = back(); pop_back(); return v; }
  size_t idaapi print(char *buf, size_t bufsize) const;
  const char *idaapi dstr(void) const;
  void idaapi add_block(int nb); // insert a block before 'nb'
  void idaapi remove_block(int from, int to);
};

class intset_t : public std::set<int>
{
public:
  size_t idaapi print(char *buf, size_t bufsize) const;
  const char *idaapi dstr(void) const;
  bool has(int value) const
  {
    const_iterator p = find(value);
    const_iterator q = end();
    return p != q;
  }
};

typedef qvector<intseq_t> array_of_intseq_t;

class intmap_t : public std::map<int, int>
{
public:
  size_t idaapi print(char *buf, size_t bufsize) const;
  const char *idaapi dstr(void) const;
};

typedef std::vector<intmap_t> array_of_intmap_t;

//-------------------------------------------------------------------------
// set of graph nodes
class node_set_t : public intset_t
{
public:
  idaapi node_set_t(void) {}
  idaapi node_set_t(int node) { insert(node); }
  idaapi node_set_t(const gdl_graph_t *g);
  bool idaapi add(int node) { return insert(node).second; }
  void idaapi sub(int node) { erase(node); }
  void idaapi sub(const node_set_t &r);
  void idaapi add(const node_set_t &r);
  bool idaapi has(int node) const
    { return (const_iterator)find(node) != end(); }
  void idaapi intersect(const node_set_t &r);
  void idaapi extract(intseq_t &out) const;
  int  idaapi first(void) const { return empty() ? -1 : *begin(); }
};

typedef std::vector<node_set_t> array_of_node_set_t;

//-------------------------------------------------------------------------
// node iterator (used to draw graphs)
class node_iterator
{
  DECLARE_HELPER(friend)
  friend class gdl_graph_t;
  const gdl_graph_t *g;
  int i;
  node_iterator &_goup(void);
  node_iterator &goup(void) { return node_iterator_goup(this); }
public:
  node_iterator(const gdl_graph_t *_g, int n) : g(_g), i(n) {}
  node_iterator &operator++(void) { i++; return goup(); }
  bool operator==(const node_iterator &n) const { return i == n.i && g == n.g; }
  bool operator!=(const node_iterator &n) const { return !(*this == n); }
  int operator*(void) const { return i; }
};

//-------------------------------------------------------------------------
// gdl graph interface - includes only functions required to draw it
class gdl_graph_t
{
  // does a path from 'm' to 'n' exist?
  bool idaapi path(node_set_t &visited, int m, int n) const;
public:
  virtual char *idaapi get_node_label(int /*n*/, char *buf, int /*bufsize*/) const { buf[0] = '\0'; return buf; }
  virtual void idaapi print_graph_attributes(FILE * /*fp*/) const {}
  virtual bool idaapi print_node(FILE * /*fp*/, int /*n*/) const { return false; }
  virtual bool idaapi print_edge(FILE * /*fp*/, int /*i*/, int /*j*/) const { return false; }
  virtual void idaapi print_node_attributes(FILE * /*fp*/, int /*n*/) const {}
  virtual idaapi ~gdl_graph_t(void) {}
  virtual int  idaapi size(void) const = 0;                    // number of the max node number
  virtual int  idaapi node_qty(void) const { return size(); }  // number of alive nodes
  virtual bool idaapi exists(int /*node*/) const { return true; }
  virtual int  idaapi entry(void) const { return 0; }
  virtual int  idaapi exit(void) const { return size()-1; }
  virtual int  idaapi nsucc(int node) const = 0;
  virtual int  idaapi npred(int node) const = 0;
  virtual int  idaapi succ(int node, int i) const = 0;
  virtual int  idaapi pred(int node, int i) const = 0;
  virtual bool idaapi empty(void) const { return node_qty() == 0; }
  virtual bgcolor_t idaapi get_node_color(int /*n*/) const { return DEFCOLOR; }
  virtual bgcolor_t idaapi get_edge_color(int /*i*/, int /*j*/) const { return DEFCOLOR; }
          void idaapi gen_gdl(FILE *fp) const;
          void idaapi gen_gdl(const char *file) const;
          size_t idaapi nedge(int node, bool ispred) const { return ispred ? npred(node) : nsucc(node); }
          int  idaapi edge(int node, int i, bool ispred) const { return ispred ? pred(node, i) : succ(node, i); }
          int  idaapi front(void) { return *begin(); }
  node_iterator idaapi begin(void) const { return node_iterator(this, 0).goup(); }
  node_iterator idaapi end(void)   const { return node_iterator(this, size()); }
  // does a path from 'm' to 'n' exist?
  bool idaapi path_exists(int m, int n) const { node_set_t v; return path(v, m, n); }
};


// Create GDL file for graph
idaman void ida_export gen_gdl(const gdl_graph_t *g, const char *fname);


// Display GDL file by calling wingraph32
// The exact name of the grapher is taken from the configuration file
// and set up by setup_graph_subsystem()
// Returns error code from os, 0-ok

idaman int ida_export display_gdl(const char *fname);


//-------------------------------------------------------------------------
// Build and display program graphs

// Build and display a flow graph
//      filename    - output file name. the file extension is not used. maybe NULL.
//      title       - graph title
//      pfn         - function to graph
//      ea1, ea2    - if pfn == NULL, then the address range
//      gflags      - combination of: CHART_PRINT_NAMES, CHART_GEN..., CHART_WINGRAPH
//                    if none of CHART_GEN_DOT, CHART_GEN_GDL, CHART_WINGRAPH
//                    is specified, the function will return false
// returns: success. if fails, a warning message is displayed on the screen

idaman bool ida_export gen_flow_graph(
        const char *filename,
        const char *title,
        func_t *pfn,
        ea_t ea1, ea_t ea2,
        int gflags);

#define CHART_PRINT_NAMES 0x1000 // print labels for each block?
#define CHART_GEN_DOT     0x2000 // generate .dot file (file extension is forced to .dot)
                                 // (not implemented yet)
#define CHART_GEN_GDL     0x4000 // generate .gdl file (file extension is forced to .gdl)
#define CHART_WINGRAPH    0x8000 // call wingraph32 to display the graph


// Build and display a simple function call graph
//      filename    - output file name. the file extension is not used. maybe NULL.
//      wait        - message to display during graph building
//      title       - graph title
//      gflags      - combination of: CHART_NOLIBFUNCS, CHART_GEN_..., CHART_WINGRAPH
//                    if none of CHART_GEN_DOT, CHART_GEN_GDL, CHART_WINGRAPH
//                    is specified, the function will return false
// returns: success. if fails, a warning message is displayed on the screen

idaman bool ida_export gen_simple_call_chart(
        const char *filename,
        const char *wait,
        const char *title,
        int gflags);

#define CHART_NOLIBFUNCS 0x0400 // don't include library functions in the graph


// Build and display a complex xref graph
//      filename    - output file name. the file extension is not used. maybe NULL.
//      wait        - message to display during graph building
//      title       - graph title
//      ea1, ea2    - address range
//      flags       - combination of CHART_... constants (see below)
//                    if none of CHART_GEN_DOT, CHART_GEN_GDL, CHART_WINGRAPH
//                    is specified, the function will return false
//      recursion_depth - optional limit of recursion
// returns: success. if fails, a warning message is displayed on the screen

idaman bool ida_export gen_complex_call_chart(
        const char *filename,
        const char *wait,
        const char *title,
        ea_t ea1,
        ea_t ea2,
        int flags,
        int32 recursion_depth=-1);

#define CHART_REFERENCING      0x0001 // references to the addresses in the list
#define CHART_REFERENCED       0x0002 // references from the addresses in the list
#define CHART_RECURSIVE        0x0004 // analyze added blocks
#define CHART_FOLLOW_DIRECTION 0x0008 // analyze references to added blocks only in the direction of the reference who discovered the current block
#define CHART_IGNORE_XTRN      0x0010
#define CHART_IGNORE_DATA_BSS  0x0020
#define CHART_IGNORE_LIB_TO    0x0040 // ignore references to library functions
#define CHART_IGNORE_LIB_FROM  0x0080 // ignore references from library functions
#define CHART_PRINT_COMMENTS   0x0100
#define CHART_PRINT_DOTS       0x0200 // print dots if xrefs exist outside of the range recursion depth


// Setup the user-defined graph colors and graph viewer program.
// This function is called by the GUI at the beginning, so no need to call
// it again.

idaman void ida_export setup_graph_subsystem(const char *grapher, bgcolor_t (idaapi *get_graph_color)(int color));


//--------------------------------------------------------------------------
//
//      The following definitions are for the kernel and the user interface only.
//      Third party plugins or modules should not use them.
//

class cancellable_graph_t : public gdl_graph_t
{
public:
  mutable bool cancelled;
  cancellable_graph_t(void) : cancelled(false) {}
  bool idaapi check_cancel(void) const;
};

//--------------------------------------------------------------------------
struct qbasic_block_t : public area_t
{
  intseq_t succ; // list of node successors
  intseq_t pred; // list of node predecessors
};

inline bool is_noret_block(fc_block_type_t btype)
{
  return btype == fcb_noret || btype == fcb_enoret;
}

inline bool is_ret_block(fc_block_type_t btype)
{
  return btype == fcb_ret || btype == fcb_cndret;
}

class qflow_chart_t : public cancellable_graph_t
{
public:
  typedef qvector<qbasic_block_t> blocks_t;
  DECLARE_HELPER(friend)
  qstring title;
  area_t bounds;
  func_t *pfn;
  int flags;
#define FC_PRINT 0x0001 // print names (used only by display_flow_chart())
#define FC_NOEXT 0x0002 // do not compute external blocks
#define FC_PREDS 0x0004 // compute predecessor lists
#define FC_APPND 0x0008 // multirange flowchart (set by append_to_flowchart)
  blocks_t blocks;
  int nproper;          // number of basic blocks belonging to the specified area

  idaapi qflow_chart_t(void) : flags(0) {}
  idaapi qflow_chart_t(const char *_title, func_t *_pfn, ea_t _ea1, ea_t _ea2, int _flags)
    : title(_title), pfn(_pfn), bounds(_ea1, _ea2), flags(_flags)
  {
    refresh();
  }
  void idaapi create(const char *_title, func_t *_pfn, ea_t _ea1, ea_t _ea2, int _flags)
  {
    title  = _title;
    pfn    = _pfn;
    bounds = area_t(_ea1, _ea2);
    flags  = _flags;
    refresh();
  }
  void idaapi create(const char *_title, const areavec_t &ranges, int _flags)
  {
    title  = _title;
    flags  = _flags;
    create_multirange_qflow_chart(*this, ranges);
  }
  void idaapi append_to_flowchart(ea_t ea1, ea_t ea2) { ::append_to_flowchart(*this, ea1, ea2); }
  void idaapi refresh(void) { create_qflow_chart(*this); }
  fc_block_type_t calc_block_type(size_t blknum) const
    { return fc_calc_block_type(*this, blknum); }
  bool is_ret_block(size_t blknum) const { return ::is_ret_block(calc_block_type(blknum)); }
  bool is_noret_block(size_t blknum) const { return ::is_noret_block(calc_block_type(blknum)); }
  void idaapi print_node_attributes(FILE * /*fp*/, int /*n*/) const {}
  int  idaapi nsucc(int node) const { return int(blocks[node].succ.size()); }
  int  idaapi npred(int node) const { return int(blocks[node].pred.size()); } // note: specify FC_PREDS for preds!
  int  idaapi succ(int node, int i) const { return blocks[node].succ[i]; }
  int  idaapi pred(int node, int i) const { return blocks[node].pred[i]; } // note: specify FC_PREDS for preds!
  bool idaapi print_names(void) const { return (flags & FC_PRINT) != 0; }
  char *idaapi get_node_label(int /*n*/, char* /*buf*/, int /*bufsize*/) const { return NULL; }
  int  idaapi size(void) const { return int(blocks.size()); }
};

//--------------------------------------------------------------------------
// The definitions below are obsolete and should not be used
#if !defined(NO_OBSOLETE_FUNCS) || defined(DEFINE_OBSOLETE_FLOW_CHART)
class graph_intseq_t : public std::vector<int>
{
public:
  void add(int x) { push_back(x); }
  void add_unique(int x)
  {
    if ( std::find(begin(), end(), x) == end() )
      add(x);
  }
};

struct basic_block_t : public area_t
{
  graph_intseq_t succ;  // we cheat here: we know that gen_gdl uses only succ
                        // and doesn't use pred. So we don't maintain preds at all
};

typedef std::map<ea_t, int> bn_t;    // block numbers

class flow_chart_t;
idaman void ida_export create_flow_chart(flow_chart_t &);

#ifdef __X64__
#pragma pack(push, 8)
#endif
class flow_chart_t : public cancellable_graph_t
{
  friend void ida_export create_flow_chart(flow_chart_t &);
public:
  typedef std::vector<basic_block_t> blocks_t;
  qstring title;
  area_t bounds;
  func_t *pfn;
  int flags;
#define FC_PRINT 0x0001 // print names
#define FC_NOEXT 0x0002 // do not compute external blocks
  blocks_t blocks;
  bn_t bn;              // block numbers
  int nproper;          // number of basic blocks belonging to the specified area

  idaapi flow_chart_t(void) : flags(0) {}
  idaapi flow_chart_t(const char *_title, func_t *_pfn, ea_t _ea1, ea_t _ea2, bool _print_names)
    : title(_title), pfn(_pfn), bounds(_ea1, _ea2), flags(_print_names ? FC_PRINT : 0)
  {
    refresh();
  }
  void idaapi create(const char *_title, func_t *_pfn, ea_t _ea1, ea_t _ea2, bool _print_names)
  {
    title = _title;
    pfn   = _pfn;
    bounds = area_t(_ea1, _ea2);
    if ( _print_names )
      flags |= FC_PRINT;
    refresh();
  }
  void idaapi refresh(void) { create_flow_chart(*this); }
  void idaapi print_graph_attributes(FILE * /*fp*/) const;
  bool idaapi print_node(FILE * /*fp*/, int /*n*/) const;
  bool idaapi print_edge(FILE * /*fp*/, int /*i*/, int /*j*/) const;
  void idaapi print_node_attributes(FILE * /*fp*/, int /*n*/) const {}
  int  idaapi nsucc(int node) const { return (int)blocks[node].succ.size(); }
  int  idaapi npred(int /*node*/) const { return 0; }
  int  idaapi succ(int node, int i) const { return blocks[node].succ[i]; }
  int  idaapi pred(int /*node*/, int /*i*/) const { return -1; }
  bool idaapi print_names(void) const { return (flags & FC_PRINT) != 0; }
  char *idaapi get_node_label(int /*n*/, char* /*buf*/, int /*bufsize*/) const { return NULL; }
  int  idaapi size(void) const { return int(blocks.size()); }
};
#ifdef __X64__
#pragma pack(pop)
#endif

idaman bool ida_export display_flow_graph(const char *title,
                                          func_t *pfn,
                                          ea_t ea1, ea_t ea2,
                                          bool print_names);
idaman bool ida_export display_simple_call_chart(const char *wait,
                                                 const char *title,
                                                 bool ignore_libfuncs);
idaman bool ida_export display_complex_call_chart(const char *wait,
                                                  const char *title,
                                                  ea_t ea1,
                                                  ea_t ea2,
                                                  int flags,
                                                  int32 recursion_depth=-1);

#endif

#pragma pack(pop)
#endif

