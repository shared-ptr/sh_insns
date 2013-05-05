
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <array>

// ----------------------------------------------------------------------------

#define make_property_class(name)\
  struct name \
  {\
    inline name (const char* val) : value (val) { } \
    const char* value; \
  };

make_property_class (format)
make_property_class (abstract)
make_property_class (code)
make_property_class (description)
make_property_class (note)
make_property_class (operation)
make_property_class (example)
make_property_class (exceptions)


enum isa
{
  SH_NONE = 0,

  SH1,
  SH2,
  SH2E,
  SH2A,
  SH3,
  SH3E,
  SH4,
  SH4A,

  SH_DSP,

  __isa_max__,
};

const struct any_isa { } SH_ANY;

struct isa_property
{
  isa_property (void)
  {
    for (auto& i : values)
      i = "";
  }

  void operator () (void) { }

  template <typename... Args> void
  operator () (isa f, const char* val, Args&&... args)
  {
    values[(int)f] = val;
    (*this) (std::forward<Args> (args)...);
  }

  template <typename... Args> void
  operator () (any_isa, const char* val, Args&&... args)
  {
    for (auto& v : values)
      v = val;
    (*this) (std::forward<Args> (args)...);
  }

  template <typename... Args> isa_property (const Args&... args)
  {
    for (auto& i : values)
      i = "";
    (*this) (args...);
  }

  const char* operator[] (isa i) const { return values[(int)i]; }

  std::array<const char*, __isa_max__> values;
};

struct group : public isa_property
{
  template <typename... Args> group (Args&&... args)
  : isa_property (std::forward<Args> (args)...)
  {
  }
};

struct issue : public isa_property
{
  template <typename... Args> issue (Args&&... args)
  : isa_property (std::forward<Args> (args)...)
  {
  }
};

struct latency : public isa_property
{
  template <typename... Args> latency (Args&&... args)
  : isa_property (std::forward<Args> (args)...)
  {
  }
};

isa_property isa_name = isa_property
(
  SH1, "SH1",
  SH2, "SH2",
  SH2E, "SH2E",
  SH2A, "SH2A",
  SH3, "SH3",
  SH3E, "SH3E",
  SH4, "SH4",
  SH4A, "SH4A",
  SH_DSP, "DSP"
);

const struct privileged_tag { } privileged;

struct t_bit
{
  t_bit (void) { }

  t_bit (const char* note)
  : note_ (note)
  {
  }

  const char* note_ = "";
};

struct dc_bit
{
  dc_bit (void) { }
  dc_bit (const char* note)
  : note_ (note)
  {
  }
  const char* note_ = "";
};

struct insn
{
  void operator () (void) { }
  void operator () (const format& p) { format_ = p.value; }
  void operator () (const abstract& p) { abstract_ = p.value; }
  void operator () (const code& p) { code_ = p.value; }
  void operator () (const description& p) { description_ = p.value; }
  void operator () (const note& p) { note_ = p.value; }
  void operator () (const operation& p) { operation_ = p.value; }
  void operator () (const example& p) { example_ = p.value; }
  void operator () (const exceptions& p) { exceptions_ = p.value; }
  void operator () (isa i) { isa_ |= (1 << (int)i); }
  void operator () (any_isa) { isa_ = -1 & ~((1 << (int)SH_DSP) | (1 << (int)SH2E) | (1 << (int)SH3E)); }
  void operator () (const group& g) { group_ = g; }
  void operator () (const issue& i) { issue_ = i; }
  void operator () (const latency& l) { latency_ = l; }
  void operator () (privileged_tag) { privileged_ = true; }
  void operator () (const t_bit& t) { t_bit_ = t; }
  void operator () (const dc_bit& dc) { dc_bit_ = dc; }

  template <typename T> void operator () (const T&);

  template <typename T, typename... Args> void
  operator () (const T& a, const Args&... args)
  {
    (*this) (a);
    (*this) (args...);
  }

  template <typename... Args> insn (const char* format, const Args&... args)
  : format_ (format)
  {
    (*this) (args...);
  }

  bool is_isa (isa i) const { return (isa_ & (1 << (int)i)) != 0; }

  int isa_ = 0;
  bool privileged_ = false;

  t_bit t_bit_;
  dc_bit dc_bit_;

  const char* format_ = "";
  const char* abstract_ = "";
  const char* code_ = "";

  group group_;
  issue issue_;
  latency latency_;

  const char* description_ = "";
  const char* note_ = "";
  const char* operation_ = "";
  const char* example_ = "";
  const char* exceptions_ = "";
};

struct insns : public std::vector<insn>
{
  void operator () (void) { }
  void operator () (const insn& i) { push_back (i); }

  template <typename T> void operator () (const T&);

  template <typename T, typename... Args> void
  operator () (const T& a, const Args&... args)
  {
    (*this) (a);
    (*this) (args...);
  }

  template <typename... Args> insns (const char* title, const Args&... args)
  : title_ (title)
  {
    (*this) (args...);
  }

  const char* title_;
};

const insn dummy_insn ("", SH1, SH2, SH2E, SH2A, SH3, SH3E, SH4, SH4A, SH_DSP);

// ----------------------------------------------------------------------------

std::vector<insns> insn_blocks;

void build_insn_blocks (void);

// ----------------------------------------------------------------------------

void print_note (const char* name, const char* val)
{
  if (std::strlen (val) == 0)
    return;

  std::cout << "<i><b>" << name << "</i></b><br/>"
	    << val << "<br/><br/>" "\n\n";
}

std::string isa_prop_str (const char* p)
{
  std::string s = p;
  if (s.size () < 6)
   s += std::string (6 - s.size (), ' ');
  return std::move (s);
}

std::string print_isa_props (const insn& i, const isa_property& p)
{
  // this one defines the order of the ISA matrices.
  std::string r;

  r += isa_prop_str (i.is_isa (SH1) ? p.values[SH1] : "");
  r += isa_prop_str (i.is_isa (SH2) ? p.values[SH2] : "");
  r += isa_prop_str (i.is_isa (SH2E) ? p.values[SH2E] : "");
  r += '\n';
  r += isa_prop_str (i.is_isa (SH3) ? p.values[SH3] : "");
  r += isa_prop_str (i.is_isa (SH3E) ? p.values[SH3E] : "");
  r += isa_prop_str (i.is_isa (SH_DSP) ? p.values[SH_DSP] : "");
  r += '\n';
  r += isa_prop_str (i.is_isa (SH4) ? p.values[SH4] : "");
  r += isa_prop_str (i.is_isa (SH4A) ? p.values[SH4A] : "");
  r += isa_prop_str (i.is_isa (SH2A) ? p.values[SH2A] : "");

  return std::move (r);
}

std::string print_isa_compatibility (const insn& i)
{
  std::string r;

  r += isa_prop_str (i.is_isa (SH1) ? isa_name[SH1] : "");
  r += isa_prop_str (i.is_isa (SH2) ? isa_name[SH2] : "");
  r += isa_prop_str (i.is_isa (SH2E) ? isa_name[SH2E] : "");
  r += '\n';
  r += isa_prop_str (i.is_isa (SH3) ? isa_name[SH3] : "");
  r += isa_prop_str (i.is_isa (SH3E) ? isa_name[SH3E] : "");
  r += isa_prop_str (i.is_isa (SH_DSP) ? isa_name[SH_DSP] : "");
  r += '\n';
  r += isa_prop_str (i.is_isa (SH4) ? isa_name[SH4] : "");
  r += isa_prop_str (i.is_isa (SH4A) ? isa_name[SH4A] : "");
  r += isa_prop_str (i.is_isa (SH2A) ? isa_name[SH2A] : "");

  if (i.privileged_)
  {
    r += '\n';
    r += "Privileged";
  }

  return std::move (r);
}

std::string print_t_bit_dc_bit_note (const insn& i)
{
  std::string r = i.t_bit_.note_;
  r += "\n";
  r += i.dc_bit_.note_;
/*
  if (r.empty ())
    return " ";
  else */
    return std::move (r);
}


int main (void)
{
  std::cout << R"html(

<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head><title>Renesas SH Instruction Set Summary</title></head>

<style type="text/css">

body
{
  font-size: 13px;
  background-color: white;
}

/* Compatibility column.  */
div.col_head_1, div.col_cont_1
{
  width: 120px;
  float: left;
  padding-right: 20px;
}

/* Format column.  */
div.col_head_2, div.col_cont_2
{
  width: 220px;
  float: left;
  padding-right: 20px;
}

/* Abstract column.  */
div.col_head_3, div.col_cont_3
{
  width: 300px;
  float: left;
  padding-right: 30px;
}

/* Code column.  */
div.col_head_4, div.col_cont_4
{
  width: 130px;
  float: left;
  padding-right: 20px;
}

/* T Bit / DC Bit column.  */
div.col_head_5, div.col_cont_5
{
  width: 60px;
  float: left;
  padding-right: 20px;
}

/* Instruction Group column.  */
div.col_head_6, div.col_cont_6
{
  width: 100px;
  float: left;
  padding-right: 20px;
}

/* Issue Cycles column.  */
div.col_head_7, div.col_cont_7
{
  width: 100px;
  float: left;
  padding-right: 20px;
}

/* Latency Cycles column.  */
div.col_head_8, div.col_cont_8
{
  width: 100px;
  float: left;
/*  padding-right: 5px; */
}

div.col_cont_1, div.col_cont_2, div.col_cont_3,
div.col_cont_4, div.col_cont_5, div.col_cont_6,
div.col_cont_7, div.col_cont_8
{
  font-family: monospace;
  white-space: pre-wrap;
  font-size: 11px;
}

div.col_cont_1, div.col_cont_6,
div.col_cont_7, div.col_cont_8
{
  font-size: 9px;
}

div.col_cont_1
{
  color: #808080;
}

div.page_head
{
  overflow: hidden;
  padding-top: 8px;
  position: fixed;
  top: 0;
  z-index: 100;

  background-color: white;
}

div.page_head_cont
{
/*  background-color: #00D000; */
  font-size: 10px;
}

div.col_head
{
  padding-right: 5px;
  background-color: #D0D0D0;
  float: left;
}

div.col_cont
{
  overflow: hidden;
  padding-bottom: 5px;
  padding-top: 5px;
  border-top-width: 1px;
  border-top-style: dotted;
}

div.col_cont_note
{
  clear: both;
  padding-top: 15px;
/*  padding-left: 0px; */
/*  padding-left:240px;*/
  width: 720px;
}

div.cpu_cols
{
  font-size: 9px;
  overflow: hidden;
  font-family: monospace;
  color: #808080;
  white-space: pre-wrap;
}

div.main
{
  position: relative;
  top: 0px;
  left: 0px;
}

div.table_header
{

}

</style>

<script language="javascript">

function on_page_load ()
{

  var rules = (document.styleSheets[0].cssRules || document.styleSheets[0].rules);
  var w = 0;

  for (var i = 0; i < rules.length; ++i)
  {
    var r = rules[i];
    if (r.selectorText.indexOf ("col") !== -1
        && r.selectorText.indexOf ("col_cont_note") === -1)
    {
      var w_val = 0;
      if (r.style.width)
	w_val += parseInt (r.style.width, 10);
      if (r.style.paddingLeft)
	w_val += parseInt (r.style.paddingLeft, 10);
      if (r.style.paddingRight)
	w_val += parseInt (r.style.paddingRight, 10);

      if (r.selectorText.indexOf ("col_isa_props") !== -1)
	w_val *= 3;

      w += w_val;
    }
  }

/*
  var w = document.getElementById ("col_head").scrollWidth;
*/
  
  var main_div = document.getElementById ("main");
  main_div.style.width = (w + 0) + "px";

  var page_head_div = document.getElementById ("page_head");
  page_head_div.style.width = (w + 0) + 'px';

  var page_head_div_h = page_head_div.clientHeight + "px";
  main_div.style.top = page_head_div_h;
}

function on_page_unload ()
{
}

function on_mouse_over (div_obj)
{
  div_obj.style.backgroundColor = "#F0F0F0";
}

function on_mouse_out (div_obj)
{
  div_obj.style.backgroundColor = "";
}

var cur_opened = null;

function on_mouse_click (div_obj)
{
  var e = div_obj.children[8];

  if (e.style.display == 'block')
  {
    e.style.display = 'none';
    cur_opened = null;
  }
  else
  {
    e.style.display = 'block';
    if (cur_opened != null)
      cur_opened.style.display = 'none';
    cur_opened = e;
  }
}

</script>


<body onload="javascript:on_page_load()" onunload="javascript:on_page_unload()">

<div class="page_head" id="page_head">

<div class="page_head_cont" id="page_head_cont">

<div style="font-size:20px;float:left">
<b>Renesas SH Instruction Set Summary</b>
</div>

<!--
If you found a mistake please drop me a message:
<a href="mailto:olegendo@gcc.gnu.org?Subject=Renesas%20SH%20Instruction%20Set%20Summary">
olegendo@gcc.gnu.org</a>
-->

<a style="float:right"href="mailto:olegendo@gcc.gnu.org?Subject=Renesas%20SuperH%20Instruction%20Set%20Summary">Contact</a>
<br/>
<a style="float:right" href="sh_insns.cpp">Page Source</a>

</br></br>

</div>

<div class="col_head" id="col_head">

  <div class="col_head_1"><b><i>Compatibilty</b></i></div>
  <div class="col_head_2"><b><i>Format</b></i></div>
  <div class="col_head_3"><b><i>Abstract</b></i></div>
  <div class="col_head_4"><b><i>Code</b></i></div>

)html"

<< "\n<div class=\"col_head_5\"><b><i>T Bit</br>DC Bit</b></i>"
   "\n<div class=\"cpu_cols\">" << " " << "</div></div>"

   "\n<div class=\"col_head_6\"><b><i>Instruction Group</b></i>"
   "\n<div class=\"cpu_cols\">" << print_isa_props (dummy_insn, isa_name) << "</div></div>"

   "\n<div class=\"col_head_7\"><b><i>Issue Cycles</b></i>"
   "\n<div class=\"cpu_cols\">" << print_isa_props (dummy_insn, isa_name) << "</div></div>"

   "\n<div class=\"col_head_8\"><b><i>Latency Cycles</b></i>"
   "\n<div class=\"cpu_cols\">" << print_isa_props (dummy_insn, isa_name) << "</div></div>"

   "\n</div></div>" << std::endl;

  build_insn_blocks ();

  std::cout << "<div class=main id=\"main\">" << std::endl;


  for (const auto& b : insn_blocks)
  {
    std::cout << "<br/><b>" << b.title_ << "</b><br/><br/>" << std::endl;

    for (const auto& i : b)
    {
      std::cout
	<< "<div class=\"col_cont\" onmouseover=\"on_mouse_over(this);\""
	   " onmouseout=\"on_mouse_out(this);\" onclick=\"on_mouse_click(this);\">" "\n"
	<< "<div class=\"col_cont_1\">" << print_isa_compatibility (i) << "</div>" "\n"
	<< "<div class=\"col_cont_2\">" << i.format_ << "</div>" "\n"
	<< "<div class=\"col_cont_3\">" << i.abstract_ << "</div>" "\n"
	<< "<div class=\"col_cont_4\">" << i.code_ << "</div>" "\n"
	<< "<div class=\"col_cont_5\">" << print_t_bit_dc_bit_note (i) << "</div>" "\n"
	<< "<div class=\"col_cont_6\">" << print_isa_props (i, i.group_) << "</div>" "\n"
	<< "<div class=\"col_cont_7\">" << print_isa_props (i, i.issue_) << "</div>" "\n"
	<< "<div class=\"col_cont_8\">" << print_isa_props (i, i.latency_) << "</div>" "\n"
	<< "<div class=\"col_cont_note\" id=\"note\" style=\"display:none\">" "\n";

      print_note ("Description", i.description_);
      print_note ("Note", i.note_);
      print_note ("Operation", i.operation_);
      print_note ("Example", i.example_);
      print_note ("Possible Exceptions", i.exceptions_);

      std::cout << "</div></div>" << std::endl;
    }
  }

  std::cout << "</div></body></html>" << std::endl;
  return 0;
}


// ----------------------------------------------------------------------------

void build_insn_blocks (void)
{

__sexpr (insn_blocks.push_back
(insns "Data Transfer Instructions"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov	#imm,Rn"
  SH_ANY
  (abstract "imm -> sign extension -> Rn")
  (code "1110nnnniiiiiiii")

  (group SH4 "EX" SH4A "MT")
  (issue SH_ANY "1")
  (latency SH_ANY "1")

  (description {R"(
    Stores immediate data, which has been sign-extended to a 32-bit value,
    into general register Rn. )"})
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.w	@(disp,PC),Rn"
  SH_ANY
  (abstract "(disp*2 + PC + 4) -> sign extension -> Rn")
  (code "1001nnnndddddddd")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.l	@(disp,PC),Rn"
  SH_ANY
  (abstract "(disp*4 + (PC & 0xFFFFFFFC) + 4) -> sign extension -> Rn")
  (code "1101nnnndddddddd")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "2")
)


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov	Rm,Rn"
  SH_ANY
  (abstract "Rm -> Rn")
  (code "0110nnnnmmmm0011")

  (group SH4 "MT" SH4A "MT")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH2A "0" SH4 "0" SH4A "1")

  (description {R"(
    Copies the contents of general register Rm to the general register Rn.
    The source register is not modified. )"})
/*
  (note {R"(
    This is a note. )"})

  (operation {R"(
    Operation pseudo code follows here. )"})

  (example {R"(
    Example code follows here. )"})

  (exceptions {R"(
    <li>Invalid operation</li> )"})
*/
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.b	Rm,@Rn"
  SH_ANY
  (abstract "Rm -> (Rn)")
  (code "0010nnnnmmmm0000")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "0" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.w	Rm,@Rn"
  SH_ANY
  (abstract "Rm -> (Rn)")
  (code "0010nnnnmmmm0001")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "0" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.l	Rm,@Rn"
  SH_ANY
  (abstract "Rm -> (Rn)")
  (code "0010nnnnmmmm0010")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "0" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.b	@Rm,Rn"
  SH_ANY
  (abstract "(Rm) -> sign extension -> Rn")
  (code "0110nnnnmmmm0000")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.w	@Rm,Rn"
  SH_ANY
  (abstract "(Rm) -> sign extension -> Rn")
  (code "0110nnnnmmmm0001")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.l	@Rm,Rn"
  SH_ANY
  (abstract "(Rm) -> Rn")
  (code "0110nnnnmmmm0010")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.b	Rm,@-Rn"
  SH_ANY
  (abstract "Rn-1 -> Rn, Rm -> (Rn)")
  (code "0010nnnnmmmm0100")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "1/1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.w	Rm,@-Rn"
  SH_ANY
  (abstract "Rn-2 -> Rn, Rm -> (Rn)")
  (code "0010nnnnmmmm0101")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "1/1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.l	Rm,@-Rn"
  SH_ANY
  (abstract "Rn-4 -> Rn, Rm -> (Rn)")
  (code "0010nnnnmmmm0110")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "1/1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.b	@Rm+,Rn"
  SH_ANY
  (abstract "(Rm) -> sign extension -> Rn, Rm+1 -> Rm")
  (code "0110nnnnmmmm0100")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "1/2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.w	@Rm+,Rn"
  SH_ANY
  (abstract "(Rm) -> sign extension -> Rn, Rm+2 -> Rm")
  (code "0110nnnnmmmm0101")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "1/2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.l	@Rm+,Rn"
  SH_ANY
  (abstract "(Rm) -> Rn, Rm+4 -> Rm")
  (code "0110nnnnmmmm0110")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "1/2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.b	R0,@(disp,Rn)"
  SH_ANY
  (abstract "R0 -> (disp + Rn)")
  (code "10000000nnnndddd")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "0" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.w	R0,@(disp,Rn)"
  SH_ANY
  (abstract "R0 -> (disp*2 + Rn)")
  (code "10000001nnnndddd")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "0" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.l	Rm,@(disp,Rn)"
  SH_ANY
  (abstract "Rm -> (disp*4 + Rn)")
  (code "0001nnnnmmmmdddd")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "0" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.b	@(disp,Rm),R0"
  SH_ANY
  (abstract "(disp + Rm) -> sign extension -> R0")
  (code "10000100mmmmdddd")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.w	@(disp,Rm),R0"
  SH_ANY
  (abstract "(disp*2 + Rm) -> sign extension -> R0")
  (code "10000101mmmmdddd")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.l	@(disp,Rm),Rn"
  SH_ANY
  (abstract "(disp*4 + Rm) -> Rn")
  (code "0101nnnnmmmmdddd")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.b	Rm,@(R0,Rn)"
  SH_ANY
  (abstract "Rm -> (R0 + Rn)")
  (code "0000nnnnmmmm0100")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "0" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.w	Rm,@(R0,Rn)"
  SH_ANY
  (abstract "Rm -> (R0 + Rn)")
  (code "0000nnnnmmmm0101")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "0" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.l	Rm,@(R0,Rn)"
  SH_ANY
  (abstract "Rm -> (R0 + Rn)")
  (code "0000nnnnmmmm0110")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "0" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.b	@(R0,Rm),Rn"
  SH_ANY
  (abstract "(R0 + Rm) -> sign extension -> Rn")
  (code "0000nnnnmmmm1100")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.w	@(R0,Rm),Rn"
  SH_ANY
  (abstract "(R0 + Rm) -> sign extension -> Rn")
  (code "0000nnnnmmmm1101")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.l	@(R0,Rm),Rn"
  SH_ANY
  (abstract "(R0 + Rm) -> Rn")
  (code "0000nnnnmmmm1110")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.b	R0,@(disp,GBR)"
  SH_ANY
  (abstract "R0 -> (disp + GBR)")
  (code "11000000dddddddd")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "0" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.w	R0,@(disp,GBR)"
  SH_ANY
  (abstract "R0 -> (disp*2 + GBR)")
  (code "11000001dddddddd")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "0" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.l	R0,@(disp,GBR)"
  SH_ANY
  (abstract "R0 -> (disp*4 + GBR)")
  (code "11000010dddddddd")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "0" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.b	@(disp,GBR),R0"
  SH_ANY
  (abstract "(disp + GBR) -> sign extension -> R0")
  (code "11000100dddddddd")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.w	@(disp,GBR),R0"
  SH_ANY
  (abstract "(disp*2 + GBR) -> sign extension -> R0")
  (code "11000101dddddddd")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.l	@(disp,GBR),R0"
  SH_ANY
  (abstract "(disp*4 + GBR) -> R0")
  (code "11000110dddddddd")

  (group SH4A "LS" SH4 "LS")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.b	R0,@Rn+"
  SH2A
  (abstract "R0 -> (Rn), Rn+1 -> Rn")
  (code "0100nnnn10001011")

  (issue SH2A "1")
  (latency SH2A "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.w	R0,@Rn+"
  SH2A
  (abstract "R0 -> (Rn), Rn+2 -> Rn")
  (code "0100nnnn10011011")

  (issue SH2A "1")
  (latency SH2A "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.l	R0,@Rn+"
  SH2A
  (abstract "R0 -> (Rn), Rn+4 -> Rn")
  (code "0100nnnn10101011")

  (issue SH2A "1")
  (latency SH2A "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.b	@-Rm,R0"
  SH2A
  (abstract "Rm-1 -> Rm, (Rm) -> sign extension -> R0")
  (code "0100mmmm11001011")

  (issue SH2A "1")
  (latency SH2A "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.w	@-Rm,R0"
  SH2A
  (abstract "Rm-2 -> Rm, (Rm) -> sign extension -> R0")
  (code "0100mmmm11011011")

  (issue SH2A "1")
  (latency SH2A "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.l	@-Rm,R0"
  SH2A
  (abstract "Rm-4 -> Rm, (Rm) -> R0")
  (code "0100mmmm11101011")

  (issue SH2A "1")
  (latency SH2A "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.b	Rm,@(disp12,Rn)"
  SH2A
  (abstract "Rm -> (disp + Rn)")
  (code "0011nnnnmmmm0001 0000dddddddddddd")

  (issue SH2A "1")
  (latency SH2A "0")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.w	Rm,@(disp12,Rn)"
  SH2A
  (abstract "Rm -> (disp*2 + Rn)")
  (code "0011nnnnmmmm0001 0001dddddddddddd")

  (issue SH2A "1")
  (latency SH2A "0")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.l	Rm,@(disp12,Rn)"
  SH2A
  (abstract "Rm -> (disp*4 + Rn)")
  (code "0011nnnnmmmm0001 0010dddddddddddd")

  (issue SH2A "1")
  (latency SH2A "0")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.b	@(disp12,Rm),Rn"
  SH2A
  (abstract "(disp + Rm) -> sign extension -> Rn")
  (code "0011nnnnmmmm0001 0100dddddddddddd")

  (issue SH2A "1")
  (latency SH2A "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.w	@(disp12,Rm),Rn"
  SH2A
  (abstract "(disp*2 + Rm) -> sign extension -> Rn")
  (code "0011nnnnmmmm0001 0101dddddddddddd")

  (issue SH2A "1")
  (latency SH2A "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mov.l	@(disp12,Rm),Rn"
  SH2A
  (abstract "(disp*4 + Rm) -> Rn")
  (code "0011nnnnmmmm0001 0110dddddddddddd")

  (issue SH2A "1")
  (latency SH2A "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mova	@(disp,PC),R0"
  SH_ANY
  (abstract "(disp*4) + (PC & 0xFFFFFFFC) + 4 -> R0")
  (code "11000111dddddddd")

  (group SH4 "EX" SH4A "LS")
  (issue SH_ANY "1")
  (latency SH_ANY "1")

  (description {R"(
    Stores the effective address of the source operand into general register R0.
    The 8-bit displacement is zero-extended and quadrupled.  Consequently, the
    relative interval from the operand is PC + 1020 bytes.  The PC is the
    address four bytes after this instruction, but the lowest two bits of the
    PC are fixed at 00. )"})

  (note {R"(
    <u>SH1*, SH2*, SH3*</u><br/>
    If this instruction is placed immediately after a
    delayed branch instruction, the PC must point to an address specified by
    (the starting address of the branch destination) + 2.<br/><br/>

    <u>SH4*</u><br/>
    If this instruction is executed in a delay slot, a slot illegal instruction
    exception will be generated. )"})

  (exceptions {R"(
    <li>Slot illegal instruction</li> )"})
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movi20	#imm20,Rn"
  SH2A
  (abstract "imm -> sign extension -> Rn")
  (code "0000nnnniiii0000 iiiiiiiiiiiiiiii")

  (issue SH2A "1")
  (latency SH2A "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movi20s	#imm20,Rn"
  SH2A
  (abstract "imm << 8 -> sign extension -> Rn")
  (code "0000nnnniiii0001 iiiiiiiiiiiiiiii")

  (issue SH2A "1")
  (latency SH2A "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movco.l	R0,@Rn"
  SH4A
  (abstract "LDST -> T\nif (T == 1) R0 -> Rn\n0 -> LDST")
  (code "0000nnnn01110011")
  (t_bit "LDST")

  (group SH4A "CO")
  (issue SH4A "1")
  (latency SH4A "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movli.l	@Rm,R0"
  SH4A
  (abstract "1 -> LDST\n(Rm) -> R0\nWhen interrupt/exception occured 0 -> LDST")
  (code "0000mmmm01100011")

  (group SH4A "CO")
  (issue SH4A "1")
  (latency SH4A "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movua.l	@Rm,R0"
  SH4A
  (abstract "(Rm) -> R0\nLoad non-boundary alignment data")
  (code "0100mmmm10101001")

  (group SH4A "LS")
  (issue SH4A "2")
  (latency SH4A "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movua.l	@Rm+,R0"
  SH4A
  (abstract "(Rm) -> R0, Rm + 4 -> Rm\nLoad non-boundary alignment data")
  (code "0100mmmm11101001")

  (group SH4A "LS")
  (issue SH4A "2")
  (latency SH4A "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movml.l	Rm,@-R15"
  SH2A
  (abstract
{ R"(R15-4 -> R15, Rm -> (R15)
R15-4 -> R15, Rm-1 -> (R15)
...
...
R15 - 4 -> R15, R0 -> (R15)
Note: When Rm = R15, read Rm as PR)"})

  (code "0100mmmm11110001")
  (issue SH2A "1-16")
  (latency SH2A "1-16")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movml.l	@R15+,Rn"
  SH2A
  (abstract
{R"((R15) -> R0, R15+4 -> R15
(R15) -> R1, R15+4 -> R15
...
...
(R15) -> Rn
Note: When Rn = R15, read Rn as PR)"})

  (code "0100nnnn11110101")
  (issue SH2A "1-16")
  (latency SH2A "2-17")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movmu.l	Rm,@-R15"
  SH2A
  (abstract
{R"(R15-4 -> R15, PR -> (R15)
R15-4 -> R15, R14 -> (R15)
...
...
R15-4 -> R15, Rm -> (R15)
Note: When Rm = R15, read Rm as PR)"})

  (code "0100mmmm11110000")
  (issue SH2A "1-16")
  (latency SH2A "1-16")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movmu.l	@R15+,Rn"
  SH2A
  (abstract
{R"((R15) -> Rn, R15+4 -> R15
(R15) -> Rn+1, R15+4 -> R15
...
...
(R15) -> R14, R15+4 -> R15
(R15) -> PR
Note: When Rn = R15, read Rn as PR)"})

  (code "0100nnnn11110100")
  (issue SH2A "1-16")
  (latency SH2A "2-17")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movrt	Rn"
  SH2A
  (abstract "~T -> Rn")
  (code "0000nnnn00111001")

  (issue SH2A "1")
  (latency SH2A "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movt	Rn"
  SH_ANY
  (abstract "T -> Rn")
  (code "0000nnnn00101001")

  (group SH4 "EX" SH4A "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movu.b	@(disp12,Rm),Rn"
  SH2A
  (abstract "(disp + Rm) -> zero extension -> Rn")
  (code "0011nnnnmmmm0001 1000dddddddddddd")

  (issue SH2A "1")
  (latency SH2A "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movu.w	@(disp12,Rm),Rn"
  SH2A
  (abstract "(disp*2 + Rm) -> zero extension -> Rn")
  (code "0011nnnnmmmm0001 1001dddddddddddd")

  (issue SH2A "1")
  (latency SH2A "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "nott"
  SH2A
  (abstract "~T -> T")
  (code "0000000001101000")
  (t_bit "~T")

  (issue SH2A "1")
  (latency SH2A "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "swap.b	Rm,Rn"
  SH_ANY
  (abstract "Rm -> swap lower 2 bytes -> Rn")
  (code "0110nnnnmmmm1000")

  (group SH4 "EX" SH4A "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "swap.w	Rm,Rn"
  SH_ANY
  (abstract "Rm -> swap upper/lower words -> Rn")
  (code "0110nnnnmmmm1001")

  (group SH4 "EX" SH4A "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "xtrct	Rm,Rn"
  SH_ANY
  (abstract "Rm:Rn middle 32 bits -> Rn")
  (code "0010nnnnmmmm1101")

  (group SH4 "EX" SH4A "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
));


__sexpr (insn_blocks.push_back
(insns "Bit Manipulation Instructions"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "band.b	#imm3,@disp12,Rn"
  SH2A
  (abstract "(imm of (disp+Rn)) & T -> T")
  (code "0011nnnn0iii1001 0100dddddddddddd")
  (t_bit "Result")

  (issue SH2A "3")
  (latency SH2A "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "bandnot.b  #imm3,@(disp12,Rn)"
  SH2A
  (abstract "~(imm of (disp+Rn)) & T -> T")
  (code "0011nnnn0iii1001 1100dddddddddddd")
  (t_bit "Result")

  (issue SH2A "3")
  (latency SH2A "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "bclr.b	#imm3,@(disp12,Rn)"
  SH2A
  (abstract "0 -> (imm of (disp+Rn))")
  (code "0011nnnn0iii1001 0000dddddddddddd")

  (issue SH2A "3")
  (latency SH2A "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "bclr	#imm3,Rn"
  SH2A
  (abstract "0 -> imm of Rn")
  (code "10000110nnnn0iii")

  (issue SH2A "1")
  (latency SH2A "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "bld.b	#imm3,@(disp12,Rn)"
  SH2A
  (abstract "(imm of (disp+Rn)) -> T")
  (code "0011nnnn0iii1001 0011dddddddddddd")
  (t_bit "Result")

  (issue SH2A "3")
  (latency SH2A "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "bld	#imm3,Rn"
  SH2A
  (abstract "imm of Rn -> T")
  (code "10000111nnnn1iii")
  (t_bit "Result")

  (issue SH2A "1")
  (latency SH2A "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "bldnot.b  #imm3,@(disp12,Rn)"
  SH2A
  (abstract "~(imm of (disp+Rn)) -> T")
  (code "0011nnnn0iii1001 1011dddddddddddd")
  (t_bit "Result")

  (issue SH2A "3")
  (latency SH2A "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "bor.b	#imm3,@(disp12,Rn)"
  SH2A
  (abstract "(imm of (disp+Rn)) | T -> T")
  (code "0011nnnn0iii1001 0101dddddddddddd")
  (t_bit "Result")

  (issue SH2A "3")
  (latency SH2A "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "bornot.b  #imm3,@(disp12,Rn)"
  SH2A
  (abstract "~(imm of (disp+Rn)) | T -> T")
  (code "0011nnnn0iii1001 1101dddddddddddd")
  (t_bit "Result")

  (issue SH2A "3")
  (latency SH2A "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "bset.b	#imm3,@(disp12,Rn)"
  SH2A
  (abstract "1 -> (imm of (disp+Rn))")
  (code "0011nnnn0iii1001 0001dddddddddddd")

  (issue SH2A "3")
  (latency SH2A "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "bset	#imm3,Rn"
  SH2A
  (abstract "1 -> imm of Rn")
  (code "10000110nnnn1iii")

  (issue SH2A "1")
  (latency SH2A "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "bst.b	#imm3,@(disp12,Rn)"
  SH2A
  (abstract "T -> (imm of (disp+Rn))")
  (code "0011nnnn0iii1001 0010dddddddddddd")

  (issue SH2A "3")
  (latency SH2A "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "bst	#imm3,Rn"
  SH2A
  (abstract "T -> imm of Rn")
  (code "10000111nnnn0iii")

  (issue SH2A "1")
  (latency SH2A "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "bxor.b	#imm3,@(disp12,Rn)"
  SH2A
  (abstract "(imm of (disp+Rn)) ^ T -> T")
  (code "0011nnnn0iii1001 0110dddddddddddd")
  (t_bit "Result")

  (issue SH2A "3")
  (latency SH2A "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
));


__sexpr (insn_blocks.push_back
(insns "Arithmetic Operation Instructions"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "add	Rm,Rn"
  SH_ANY
  (abstract "Rn + Rm -> Rn")
  (code "0011nnnnmmmm1100")

  (group SH4 "EX" SH4A "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "add	#imm,Rn"
  SH_ANY
  (abstract "Rn + (sign extension)imm")
  (code "0111nnnniiiiiiii")

  (group SH4 "EX" SH4A "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "addc	Rm,Rn"
  SH_ANY
  (abstract "Rn + Rm + T -> Rn, carry -> T")
  (code "0011nnnnmmmm1110")
  (t_bit "Carry")

  (group SH4 "EX" SH4A "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "addv	Rm,Rn"
  SH_ANY
  (abstract "Rn + Rm -> Rn, overflow -> T")
  (code "0011nnnnmmmm1111")
  (t_bit "Overflow")

  (group SH4 "EX" SH4A "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "cmp/eq	#imm,R0"
  SH_ANY
  (abstract "If R0 = (sign extension)imm: 1 -> T\nElse: 0 -> T")
  (code "10001000iiiiiiii")
  (t_bit "Result")

  (group SH4 "MT" SH4A "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "cmp/eq	Rm,Rn"
  SH_ANY
  (abstract "If Rn = Rm: 1 -> T\nElse: 0 -> T")
  (code "0011nnnnmmmm0000")
  (t_bit "Result")

  (group SH4 "MT" SH4A "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "cmp/hs	Rm,Rn"
  SH_ANY
  (abstract "If Rn >= Rm (unsigned): 1 -> T\nElse: 0 -> T")
  (code "0011nnnnmmmm0010")
  (t_bit "Result")

  (group SH4 "MT" SH4A "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "cmp/ge	Rm,Rn"
  SH_ANY
  (abstract "If Rn >= Rm (signed): 1 -> T\nElse: 0 -> T")
  (code "0011nnnnmmmm0011")
  (t_bit "Result")

  (group SH4 "MT" SH4A "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "cmp/hi	Rm,Rn"
  SH_ANY
  (abstract "If Rn > Rm (unsigned): 1 -> T\nElse: 0 -> T")
  (code "0011nnnnmmmm0110")
  (t_bit "Result")

  (group SH4 "MT" SH4A "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "cmp/gt	Rm,Rn"
  SH_ANY
  (abstract "If Rn > Rm (signed): 1 -> T\nElse: 0 -> T")
  (code "0011nnnnmmmm0111")
  (t_bit "Result")

  (group SH4 "MT" SH4A "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "cmp/pl	Rn"
  SH_ANY
  (abstract "If Rn > 0 (signed): 1 -> T\nElse: 0 -> T")
  (code "0100nnnn00010101")
  (t_bit "Result")

  (group SH4 "MT" SH4A "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "cmp/pz	Rn"
  SH_ANY
  (abstract "If Rn >= 0 (signed): 1 -> T\nElse: 0 -> T")
  (code "0100nnnn00010001")
  (t_bit "Result")

  (group SH4 "MT" SH4A "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "cmp/str	Rm,Rn"
  SH_ANY
  (abstract "If Rn and Rm have an equal byte: 1 -> T\nElse: 0 -> T")
  (code "0010nnnnmmmm1100")
  (t_bit "Result")

  (group SH4 "MT" SH4A "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "clips.b	Rn"
  SH2A
  (abstract "If Rn > 0x0000007F: 0x0000007F -> Rn, 1 -> CS\nIf Rn < 0xFFFFFF80: 0xFFFFFF80 -> Rn, 1 -> CS")
  (code "0100nnnn10010001")

  (issue SH2A "1")
  (latency SH2A "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "clips.w	Rn"
  SH2A
  (abstract "If Rn > 0x00007FFF: 0x00007FFF -> Rn, 1 -> CS\nIf Rn < 0xFFFF8000: 0xFFFF8000 -> Rn, 1 -> CS")
  (code "0100nnnn10010101")

  (issue SH2A "1")
  (latency SH2A "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "clipu.b	Rn"
  SH2A
  (abstract "If Rn > 0x000000FF: 0x000000FF -> Rn, 1 -> CS")
  (code "0100nnnn10000001")

  (issue SH2A "1")
  (latency SH2A "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "clipu.w	Rn"
  SH2A
  (abstract "If Rn > 0x0000FFFF: 0x0000FFFF -> Rn, 1 -> CS")
  (code "0100nnnn10000101")

  (issue SH2A "1")
  (latency SH2A "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "div1	Rm,Rn"
  SH_ANY
  (abstract "1-step division (Rn / Rm)")
  (code "0011nnnnmmmm0100")
  (t_bit "Result")

  (group SH4 "EX" SH4A "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "div0s	Rm,Rn"
  SH_ANY
  (abstract "MSB of Rn -> Q, MSB of Rm -> M, M ^ Q -> T")
  (code "0010nnnnmmmm0111")
  (t_bit "Result")

  (group SH4 "EX" SH4A "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "div0u"
  SH_ANY
  (abstract "0 -> M, 0 -> Q, 0 -> T")
  (code "0000000000011001")
  (t_bit "0")

  (group SH4 "EX" SH4A "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "divs	R0,Rn"
  SH2A
  (abstract "Signed, Rn / R0 -> Rn\n32 / 32 -> 32 bits")
  (code "0100nnnn10010100")

  (issue SH2A "36")
  (latency SH2A "36")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "divu	R0,Rn"
  SH2A
  (abstract "Unsigned, Rn / R0 -> Rn\n32 / 32 -> 32 bits")
  (code "0100nnnn10000100")

  (issue SH2A "36")
  (latency SH2A "36")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dmuls.l	Rm,Rn"
  SH2 SH2A SH3 SH4 SH4A
  (abstract "Signed, Rn * Rm -> MACH:MACL\n32 * 32 -> 64 bits")
  (code "0011nnnnmmmm1101")

  (group SH4 "CO" SH4A "EX")
  (issue SH2 "2" SH3 "2" SH4A "1" SH2A "2" SH4 "2")
  (latency SH2 "2-4" SH3 "2-5" SH4A "2" SH2A "3" SH4 "4/4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dmulu.l	Rm,Rn"
  SH2 SH2A SH3 SH4 SH4A
  (abstract "Unsigned, Rn * Rm -> MACH:MACL\n32 * 32 -> 64 bits")
  (code "0011nnnnmmmm0101")

  (group SH4A "EX" SH4 "CO")
  (issue SH2 "2" SH3 "2" SH4A "1" SH2A "2" SH4 "2")
  (latency SH2 "2-4" SH3 "2-5" SH4A "2" SH2A "2" SH4 "4/4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dt	Rn"
  SH2 SH2A SH3 SH4 SH4A
  (abstract "Rn-1 -> Rn\nIf Rn = 0: 1 -> T, else: 0 -> T")
  (code "0100nnnn00010000")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "exts.b	Rm,Rn"
  SH_ANY
  (abstract "Rm sign-extended from byte -> Rn")
  (code "0110nnnnmmmm1110")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "exts.w	Rm,Rn"
  SH_ANY
  (abstract "Rm sign-extended from word -> Rn")
  (code "0110nnnnmmmm1111")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "extu.b	Rm,Rn"
  SH_ANY
  (abstract "Rm zero-extended from byte -> Rn")
  (code "0110nnnnmmmm1100")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "extu.w	Rm,Rn"
  SH_ANY
  (abstract "Rm zero-extended from word -> Rn")
  (code "0110nnnnmmmm1101")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mac.l	@Rm+,@Rn+"
  SH2 SH2A SH3 SH4 SH4A
  (abstract "Signed, (Rn) * (Rm) + MAC -> MAC\n32 * 32 + 64 -> 64 bits")
  (code "0000nnnnmmmm1111")

  (group SH4A "CO" SH4 "CO")
  (issue SH2 "2" SH3 "2" SH4A "2" SH2A "4" SH4 "2")
  (latency SH2 "2-4" SH3 "2-5" SH4A "5" SH2A "5" SH4 "2/4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mac.w	@Rm+,@Rn+"
  SH_ANY
  (abstract "Signed, (Rn) * (Rm) + MAC -> MAC\nSH1: 16 * 16 + 42 -> 42 bits\nOther: 16 * 16 + 64 -> 64 bits")
  (code "0100nnnnmmmm1111")

  (group SH4A "CO" SH4 "CO")
  (issue SH1 "2" SH2 "2" SH3 "2" SH4A "2" SH2A "3" SH4 "2")
  (latency SH1 "2-3" SH2 "2-3" SH3 "2-5" SH4A "4" SH2A "4" SH4 "2/4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mul.l	Rm,Rn"
  SH2 SH2A SH3 SH4 SH4A
  (abstract "Rn * Rm -> MACL\n32 * 32 -> 32 bits")
  (code "0000nnnnmmmm0111")

  (group SH4A "EX" SH4 "CO")
  (issue SH2 "2" SH3 "2" SH4A "1" SH2A "2" SH4 "2")
  (latency SH2 "2-4" SH3 "2-4" SH4A "2" SH2A "3" SH4 "4/4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mulr	R0,Rn"
  SH2A
  (abstract "R0 * Rn -> Rn\n32 * 32 -> 32 bits")
  (code "0100nnnn10000000")

  (issue SH2A "2")
  (latency SH2A "4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "muls.w	Rm,Rn"
  SH_ANY
  (abstract "Signed, Rn * Rm -> MACL\n16 * 16 -> 32 bits")
  (code "0010nnnnmmmm1111")

  (group SH4A "EX" SH4 "CO")
  (issue SH1 "2" SH2 "2" SH3 "2" SH4A "1" SH2A "1" SH4 "2")
  (latency SH1 "1-3" SH2 "1-3" SH3 "1-3" SH4A "1" SH2A "2" SH4 "4/4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "mulu.w	Rm,Rn"
  SH_ANY
  (abstract "Unsigned, Rn * Rm -> MACL\n16 * 16 -> 32 bits")
  (code "0010nnnnmmmm1110")

  (group SH4A "EX" SH4 "CO")
  (issue SH1 "2" SH2 "2" SH3 "2" SH4A "1" SH2A "1" SH4 "2")
  (latency SH1 "1-3" SH2 "1-3" SH3 "1-3" SH4A "1" SH2A "2" SH4 "4/4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "neg	Rm,Rn"
  SH_ANY
  (abstract "0 - Rm -> Rn")
  (code "0110nnnnmmmm1011")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "negc	Rm,Rn"
  SH_ANY
  (abstract "0 - Rm - T -> Rn, borrow -> T")
  (code "0110nnnnmmmm1010")
  (t_bit "Borrow")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sub	Rm,Rn"
  SH_ANY
  (abstract "Rn - Rm -> Rn")
  (code "0011nnnnmmmm1000")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "subc	Rm,Rn"
  SH_ANY
  (abstract "Rn - Rm - T -> Rn, borrow -> T")
  (code "0011nnnnmmmm1010")
  (t_bit "Borrow")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "subv	Rm,Rn"
  SH_ANY
  (abstract "Rn - Rm -> Rn, underflow -> T")
  (code "0011nnnnmmmm1011")
  (t_bit "Underflow")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
));


__sexpr (insn_blocks.push_back
(insns "Logic Operation Instructions"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "and	Rm,Rn"
  SH_ANY
  (abstract "Rn & Rm -> Rn")
  (code "0010nnnnmmmm1001")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "and	#imm,R0"
  SH_ANY
  (abstract "R0 & (zero extend)imm -> R0")
  (code "11001001iiiiiiii")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "and.b	#imm,@(R0,GBR)"
  SH_ANY
  (abstract "(R0 + GBR) & (zero extend)imm -> (R0 + GBR)")
  (code "11001101iiiiiiii")

  (group SH4A "CO" SH4 "CO")
  (issue SH1 "2" SH2 "2" SH3 "2" SH4A "3" SH2A "3" SH4 "4")
  (latency SH1 "3" SH2 "3" SH3 "3" SH4A "3" SH4 "4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "not	Rm,Rn"
  SH_ANY
  (abstract "~Rm -> Rn")
  (code "0110nnnnmmmm0111")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "or	Rm,Rn"
  SH_ANY
  (abstract "Rn | Rm -> Rn")
  (code "0010nnnnmmmm1011")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "or	#imm,R0"
  SH_ANY
  (abstract "R0 | (zero extend)imm -> R0")
  (code "11001011iiiiiiii")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "or.b	#imm,@(R0,GBR)"
  SH_ANY
  (abstract "(R0 + GBR) | (zero extend)imm -> (R0 + GBR)")
  (code "11001111iiiiiiii")

  (group SH4A "CO" SH4 "CO")
  (issue SH1 "2" SH2 "2" SH3 "2" SH4A "3" SH2A "3" SH4 "4")
  (latency SH1 "3" SH2 "3" SH3 "3" SH4A "3" SH2A "2" SH4 "4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "tas.b	@Rn"
  SH_ANY
  (abstract "If (Rn) = 0: 1 -> T, else: 0 -> T\n1 -> MSB of (Rn)")
  (code "0100nnnn00011011")
  (t_bit "Result")

  (group SH4A "CO" SH4 "CO")
  (issue SH1 "2" SH2 "2" SH3 "2" SH4A "4" SH2A "3" SH4 "5")
  (latency SH1 "4" SH2 "4" SH3 "3/4" SH4A "4" SH2A "3" SH4 "5")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "tst	Rm,Rn"
  SH_ANY
  (abstract "If Rn & Rm = 0: 1 -> T, else: 0 -> T")
  (code "0010nnnnmmmm1000")
  (t_bit "Result")

  (group SH4A "EX" SH4 "MT")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "tst	#imm,R0"
  SH_ANY
  (abstract "If R0 & (zero extend)imm = 0: 1 -> T, else 0 -> T")
  (code "11001000iiiiiiii")
  (t_bit "Result")

  (group SH4A "EX" SH4 "MT")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "tst.b	#imm,@(R0,GBR)"
  SH_ANY
  (abstract "If (R0 + GBR) & (zero extend)imm = 0: 1 -> T, else 0 -> T")
  (code "11001100iiiiiiii")
  (t_bit "Result")

  (group SH4A "CO" SH4 "CO")
  (issue SH1 "2" SH2 "2" SH3 "2" SH4A "3" SH2A "3" SH4 "3")
  (latency SH1 "3" SH2 "3" SH3 "3" SH4A "3" SH2A "3" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "xor	Rm,Rn"
  SH_ANY
  (abstract "Rn ^ Rm -> Rn")
  (code "0010nnnnmmmm1010")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "xor	#imm,R0"
  SH_ANY
  (abstract "R0 ^ (zero extend)imm -> R0")
  (code "11001010iiiiiiii")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "xor.b	#imm,@(R0,GBR)"
  SH_ANY
  (abstract "(R0 + GBR) ^ (zero extend)imm -> (R0 + GBR)")
  (code "11001110iiiiiiii")

  (group SH4A "CO" SH4 "CO")
  (issue SH1 "2" SH2 "2" SH3 "2" SH4A "3" SH2A "3" SH4 "4")
  (latency SH1 "3" SH2 "3" SH3 "3" SH4A "3" SH2A "2" SH4 "4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
));


__sexpr (insn_blocks.push_back
(insns "Shift Instructions"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "rotl	Rn"
  SH_ANY
  (abstract "T << Rn << MSB")
  (code "0100nnnn00000100")
  (t_bit "MSB")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "rotr	Rn"
  SH_ANY
  (abstract "LSB >> Rn >> T")
  (code "0100nnnn00000101")
  (t_bit "LSB")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "rotcl	Rn"
  SH_ANY
  (abstract "T << Rn << T")
  (code "0100nnnn00100100")
  (t_bit "MSB")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "rotcr	Rn"
  SH_ANY
  (abstract "T >> Rn >> T")
  (code "0100nnnn00100101")
  (t_bit "LSB")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "shad	Rm,Rn"
  SH2A SH3 SH4 SH4A
  (abstract "If Rm >= 0: Rn << Rm -> Rn\nIf Rm < 0: Rn >> |Rm| -> [MSB -> Rn]")
  (code "0100nnnnmmmm1100")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "shal	Rn"
  SH_ANY
  (abstract "T << Rn << 0")
  (code "0100nnnn00100000")
  (t_bit "MSB")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "shar	Rn"
  SH_ANY
  (abstract "MSB >> Rn >> T")
  (code "0100nnnn00100001")
  (t_bit "LSB")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "shld	Rm,Rn"
  SH2A SH3 SH4 SH4A
  (abstract "If Rm >= 0: Rn << Rm -> Rn\nIf Rm < 0: Rn >> |Rm| -> [0 -> Rn]")
  (code "0100nnnnmmmm1101")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "shll	Rn"
  SH_ANY
  (abstract "T << Rn << 0")
  (code "0100nnnn00000000")
  (t_bit "MSB")

  // this is the same as shal
  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "shlr	Rn"
  SH_ANY
  (abstract "0 >> Rn >> T")
  (code "0100nnnn00000001")
  (t_bit "LSB")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "shll2	Rn"
  SH_ANY
  (abstract "Rn << 2 -> Rn")
  (code "0100nnnn00001000")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "shlr2	Rn"
  SH_ANY
  (abstract "Rn >> 2 -> [0 -> Rn]")
  (code "0100nnnn00001001")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "shll8	Rn"
  SH_ANY
  (abstract "Rn << 8 -> Rn")
  (code "0100nnnn00011000")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "shlr8	Rn"
  SH_ANY
  (abstract "Rn >> 8 -> [0 -> Rn]")
  (code "0100nnnn00011001")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "shll16	Rn"
  SH_ANY
  (abstract "Rn << 16 -> Rn")
  (code "0100nnnn00101000")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "shlr16	Rn"
  SH_ANY
  (abstract "Rn >> 16 -> [0 -> Rn]")
  (code "0100nnnn00101001")

  (group SH4A "EX" SH4 "EX")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
));


__sexpr (insn_blocks.push_back
(insns "Branch Instructions"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "bf	label"
  SH_ANY
  (abstract "If T = 0: disp*2 + PC + 4 -> PC, else: nop")
  (code "10001011dddddddd")

  (group SH4A "BR" SH4 "BR")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1-3" SH2A "1/3" SH4 "1")
  (latency SH1 "1/3" SH2 "1/3" SH3 "1/3" SH4A "1" SH2A "1/3" SH4 "1/2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "bf/s	label"
  SH2 SH2A SH3 SH4 SH4A
  (abstract "Delayed branch, if T = 0: disp*2 + PC + 4 -> PC, else: nop")
  (code "10001111dddddddd")

  (group SH4A "BR" SH4 "BR")
  (issue SH2 "1" SH3 "1" SH4A "1-3" SH2A "1/2" SH4 "1")
  (latency SH2 "1/2" SH3 "1/2" SH4A "1" SH2A "1/2" SH4 "1/2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "bt	label"
  SH_ANY
  (abstract "If T = 1: disp*2 + PC + 4 -> PC, else: nop")
  (code "10001001dddddddd")

  (group SH4A "BR" SH4 "BR")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1-3" SH2A "1/3" SH4 "1")
  (latency SH1 "1/3" SH2 "1/3" SH3 "1/3" SH4A "1" SH2A "1/3" SH4 "1/2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "bt/s	label"
  SH2 SH2A SH3 SH4 SH4A
  (abstract "Delayed branch, if T = 1: disp*2 + PC + 4 -> PC, else: nop")
  (code "10001101dddddddd")

  (group SH4A "BR" SH4 "BR")
  (issue SH2 "1" SH3 "1" SH4A "1-3" SH2A "1/2" SH4 "1")
  (latency SH2 "1/2" SH3 "1/2" SH4A "1" SH2A "1/2" SH4 "1/2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "bra	label"
  SH_ANY
  (abstract "Delayed branch, disp*2 + PC + 4 -> PC")
  (code "1010dddddddddddd")

  (group SH4A "BR" SH4 "BR")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1-3" SH2A "2" SH4 "1")
  (latency SH1 "2" SH2 "2" SH3 "2" SH4A "1" SH2A "2" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "braf	Rm"
  SH2 SH2A SH3 SH4 SH4A
  (abstract "Delayed branch, Rm + PC + 4 -> PC")
  (code "0000mmmm00100011")

  (group SH4A "BR" SH4 "CO")
  (issue SH2 "1" SH3 "1" SH4A "4" SH2A "2" SH4 "2")
  (latency SH2 "2" SH3 "2" SH4A "1" SH2A "2" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "bsr	label"
  SH_ANY
  (abstract "Delayed branch, PC + 4 -> PR, disp*2 + PC + 4 -> PC")
  (code "1011dddddddddddd")

  (group SH4A "BR" SH4 "BR")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1-3" SH2A "2" SH4 "1")
  (latency SH1 "2" SH2 "2" SH3 "2" SH4A "1" SH2A "2" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "bsrf	Rm"
  SH2 SH2A SH3 SH4 SH4A
  (abstract "Delayed branch, PC + 4 -> PR, Rm + PC + 4 -> PC")
  (code "0000mmmm00000011")

  (group SH4A "BR" SH4 "CO")
  (issue SH2 "1" SH3 "1" SH4A "4" SH2A "2" SH4 "2")
  (latency SH2 "2" SH3 "2" SH4A "1" SH2A "2" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "jmp	@Rm"
  SH_ANY
  (abstract "Delayed branch, Rm -> PC")
  (code "0100mmmm00101011")

  (group SH4A "BR" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "4" SH2A "2" SH4 "2")
  (latency SH1 "2" SH2 "2" SH3 "2" SH4A "1" SH2A "2" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "jsr	@Rm"
  SH_ANY
  (abstract "Delayed branch, PC + 4 -> PR, Rm -> PC")
  (code "0100mmmm00001011")

  (group SH4A "BR" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "4" SH2A "2" SH4 "2")
  (latency SH1 "2" SH2 "2" SH3 "2" SH4A "1" SH2A "2" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "jsr/n	@Rm"
  SH2A
  (abstract "PC + 2 -> PR, Rn -> PC")
  (code "0100mmmm01001011")

  (issue SH2A "3")
  (latency SH2A "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "jsr/n	@@(disp8,TBR)"
  SH2A
  (abstract "PC + 2 -> PR, (disp*4 + TBR) -> PC")
  (code "10000011dddddddd")

  (issue SH2A "5")
  (latency SH2A "5")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "rts"
  SH_ANY
  (abstract "Delayed branch, PR -> PC")
  (code "0000000000001011")

  (group SH4A "BR" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1-4" SH2A "2" SH4 "2")
  (latency SH1 "2" SH2 "2" SH3 "2" SH4A "1" SH2A "2" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "rts/n"
  SH2A
  (abstract "PR -> PC")
  (code "0000000001101011")

  (issue SH2A "3")
  (latency SH2A "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "rtv/n	Rm"
  SH2A
  (abstract "Rm -> R0, PR -> PC")
  (code "0000mmmm01111011")

  (issue SH2A "3")
  (latency SH2A "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
));


__sexpr (insn_blocks.push_back
(insns "System Control Instructions"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "clrmac"
  SH_ANY
  (abstract "0 -> MACH, 0 -> MACL")
  (code "0000000000101000")

  (group SH4A "EX" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "clrs"
  SH3 SH4 SH4A
  (abstract "0 -> S")
  (code "0000000001001000")

  (group SH4A "EX" SH4 "CO")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "clrt"
  SH_ANY
  (abstract "0 -> T")
  (code "0000000000001000")
  (t_bit "0")

  (group SH4A "EX" SH4 "MT")
  (issue SH_ANY "1")
  (latency SH_ANY "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "icbi	@Rn"
  SH4A
  (abstract "Invalidate instruction cache block indicated by logical address")
  (code "0000nnnn11100011")

  (group SH4A "CO")
  (issue SH4A "16")
  (latency SH4A "13")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldbank	@Rm,R0"
  SH2A
  (abstract "(Specified register bank entry) -> R0")
  (code "0100mmmm11100101")

  (issue SH2A "6")
  (latency SH2A "5")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc	Rm,SR"
  SH_ANY privileged
  (abstract "Rm -> SR")
  (code "0100mmmm00001110")
  (t_bit "LSB")

  (group SH4A "CO" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "7" SH2A "3" SH4 "4")
  (latency SH1 "1" SH2 "1" SH3 "5" SH4A "4" SH2A "2" SH4 "4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc	Rm,TBR"
  SH2A
  (abstract "Rm -> TBR")
  (code "0100mmmm01001010")

  (issue SH2A "1")
  (latency SH2A "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc	Rm,GBR"
  SH_ANY
  (abstract "Rm -> GBR")
  (code "0100mmmm00011110")

  (group SH4A "LS" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "3")
  (latency SH1 "1" SH2 "1" SH3 "1/3" SH4A "1" SH2A "1" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc	Rm,VBR"
  SH_ANY privileged
  (abstract "Rm -> VBR")
  (code "0100mmmm00101110")

  (group SH4A "LS" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH1 "1" SH2 "1" SH3 "1/3" SH4A "1" SH2A "1" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc	Rm,MOD"
  SH_DSP
  (abstract "Rm -> MOD")
  (code "0100mmmm01011110")

  (issue SH_DSP "1")
  (latency SH_DSP "1/3")

  // SH1-DSP: 1 cycle latency
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc	Rm,RE"
  SH_DSP
  (abstract "Rm -> RE")
  (code "0100mmmm01111110")

  (issue SH_DSP "1")
  (latency SH_DSP "1/3")

  // SH1-DSP: 1 cycle latency
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc	Rm,RS"
  SH_DSP
  (abstract "Rm -> RS")
  (code "0100mmmm01101110")

  (issue SH_DSP "1")
  (latency SH_DSP "1/3")

  // SH1-DSP: 1 cycle latency
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc	Rm,SGR"
  SH4A privileged
  (abstract "Rm -> SGR")
  (code "0100mmmm00111010")

  (group SH4A "CO")
  (issue SH4A "4")
  (latency SH4A "4")

  (note {R"(
    Not sure whether it is also available on SH4.
    It is not marked as new instruction for SH4A. )"})
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc	Rm,SSR"
  SH3 SH4 SH4A privileged
  (abstract "Rm -> SSR")
  (code "0100mmmm00111110")

  (group SH4A "LS" SH4 "CO")
  (issue SH3 "1" SH4A "1" SH4 "1")
  (latency SH3 "1/3" SH4A "1" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc	Rm,SPC"
  SH3 SH4 SH4A privileged
  (abstract "Rm -> SPC")
  (code "0100mmmm01001110")

  (group SH4A "LS" SH4 "CO")
  (issue SH3 "1" SH4A "1" SH4 "3")
  (latency SH3 "1/3" SH4A "1" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc	Rm,DBR"
  SH4 SH4A privileged
  (abstract "Rm -> DBR")
  (code "0100mmmm11110110")

  (group SH4A "CO" SH4 "CO")
  (issue SH4A "4" SH4 "1")
  (latency SH4A "4" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc	Rm,Rn_BANK"
  SH3 SH4 SH4A privileged
  (abstract "Rm -> Rn_BANK (n = 0-7)")
  (code "0100mmmm1nnn1110")

  (group SH4A "LS" SH4 "CO")
  (issue SH3 "1" SH4A "1" SH4 "1")
  (latency SH3 "1/3" SH4A "1" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc.l	@Rm+,SR"
  SH_ANY privileged
  (abstract "(Rm) -> SR, Rm+4 -> Rm")
  (code "0100mmmm00000111")
  (t_bit "LSB")

  (group SH4A "CO" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "2" SH4A "9" SH2A "5" SH4 "4")
  (latency SH1 "3" SH2 "3" SH3 "7" SH4A "4" SH2A "4" SH4 "4/4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc.l	@Rm+,GBR"
  SH_ANY
  (abstract "(Rm) -> GBR, Rm+4 -> Rm")
  (code "0100mmmm00010111")

  (group SH4A "LS" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "3")
  (latency SH1 "3" SH2 "3" SH3 "1/5" SH4A "1" SH2A "2" SH4 "3/3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc.l	@Rm+,VBR"
  SH_ANY privileged
  (abstract "(Rm) -> VBR, Rm+4 -> Rm")
  (code "0100mmmm00100111")

  (group SH4A "LS" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH1 "3" SH2 "3" SH3 "1/5" SH4A "1" SH2A "2" SH4 "1/3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc.l	@Rm+,MOD"
  SH_DSP
  (abstract "(Rm) -> MOD, Rm+4 -> Rm")
  (code "0100mmmm01010111")

  (issue SH_DSP "1")
  (latency SH_DSP "1/5")

  // SH1-DSP latency: 3
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc.l	@Rm+,RE"
  SH_DSP
  (abstract "(Rm) -> RE, Rm+4 -> Rm")
  (code "0100mmmm01110111")

  (issue SH_DSP "1")
  (latency SH_DSP "1/5")

  // SH1-DSP latency: 3
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc.l	@Rm+,RS"
  SH_DSP
  (abstract "(Rm) -> RS, Rm+4 -> Rm")
  (code "0100mmmm01100111")

  (issue SH_DSP "1")
  (latency SH_DSP "1/5")

  // SH1-DSP latency: 3
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc.l	@Rm+,SGR"
  SH4A privileged
  (abstract "(Rm) -> SGR, Rm+4 -> Rm")
  (code "0100mmmm00110110")

  (group SH4A "CO")
  (issue SH4A "4")
  (latency SH4A "4")

  (note {R"(
    Not sure whether it is also available on SH4.
    It is not marked as new instruction for SH4A. )"})
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc.l	@Rm+,SSR"
  SH3 SH4 SH4A privileged
  (abstract "(Rm) -> SSR, Rm+4 -> Rm")
  (code "0100mmmm00110111")

  (group SH4A "LS" SH4 "CO")
  (issue SH3 "1" SH4A "1" SH4 "1")
  (latency SH3 "1/5" SH4A "1" SH4 "1/3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc.l	@Rm+,SPC"
  SH3 SH4 SH4A privileged
  (abstract "(Rm) -> SPC, Rm+4 -> Rm")
  (code "0100mmmm01000111")

  (group SH4A "LS" SH4 "CO")
  (issue SH3 "1" SH4A "1" SH4 "1")
  (latency SH3 "1/5" SH4A "1" SH4 "1/3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc.l	@Rm+,DBR"
  SH4 SH4A privileged
  (abstract "(Rm) -> DBR, Rm+4 -> Rm")
  (code "0100mmmm11110110")

  (group SH4A "CO" SH4 "CO")
  (issue SH4A "4" SH4 "1")
  (latency SH4A "4" SH4 "1/3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldc.l	@Rm+,Rn_BANK"
  SH3 SH4 SH4A privileged
  (abstract "(Rm) -> Rn_BANK, Rm+4 -> Rm")
  (code "0100mmmm1nnn0111")

  (group SH4A "LS" SH4 "CO")
  (issue SH3 "1" SH4A "1" SH4 "1")
  (latency SH3 "1/5" SH4A "1" SH4 "1/3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldre	@(disp,PC)"
  SH_DSP
  (abstract "disp*2 + PC -> RE")
  (code "10001110dddddddd")

  (issue SH_DSP "1")
  (latency SH_DSP "3")

  // SH1-DSP latency: 1
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldrs	@(disp,PC)"
  SH_DSP
  (abstract "disp*2 + PC -> RS")
  (code "10001100dddddddd")

  (issue SH_DSP "1")
  (latency SH_DSP "3")

  // SH1-DSP latency: 1
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds	Rm,MACH"
  SH_ANY
  (abstract "Rm -> MACH")
  (code "0100mmmm00001010")

  (group SH4A "LS" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds	Rm,MACL"
  SH_ANY
  (abstract "Rm -> MACL")
  (code "0100mmmm00011010")

  (group SH4A "LS" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds	Rm,PR"
  SH_ANY
  (abstract "Rm -> PR")
  (code "0100mmmm00101010")

  (group SH4A "LS" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "2")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds	Rm,DSR"
  SH_DSP
  (abstract "Rm -> DSR")
  (code "0100mmmm01101010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds	Rm,A0"
  SH_DSP
  (abstract "Rm -> A0")
  (code "0100mmmm01110110")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds	Rm,X0"
  SH_DSP
  (abstract "Rm -> X0")
  (code "0100mmmm10001010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds	Rm,X1"
  SH_DSP
  (abstract "Rm -> X1")
  (code "0100mmmm10011010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds	Rm,Y0"
  SH_DSP
  (abstract "Rm -> Y0")
  (code "0100mmmm10101010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds	Rm,Y1"
  SH_DSP
  (abstract "Rm -> Y1")
  (code "0100mmmm10111010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds.l	@Rm+,MACH"
  SH_ANY
  (abstract "(Rm) -> MACH, Rm+4 -> Rm")
  (code "0100mmmm00000110")

  (group SH4A "LS" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "1/3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds.l	@Rm+,MACL"
  SH_ANY
  (abstract "(Rm) -> MACL, Rm+4 -> Rm")
  (code "0100mmmm00010110")

  (group SH4A "LS" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "1/3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds.l	@Rm+,PR"
  SH_ANY
  (abstract "(Rm) -> PR, Rm+4 -> Rm")
  (code "0100mmmm00100110")

  (group SH4A "LS" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "2")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "2/3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds.l	@Rm+,DSR"
  SH_DSP
  (abstract "(Rm) -> DSR, Rm+4 -> Rm")
  (code "0100mmmm01100110")

  (issue SH_DSP "1")
  (latency SH_DSP "1/5")

  // SH1-DSP latency: 1
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds.l	@Rm+,A0"
  SH_DSP
  (abstract "(Rm) -> A0, Rm+4 -> Rm")
  (code "0100mmmm01110110")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds.l	@Rm+,X0"
  SH_DSP
  (abstract "(Rm) -> X0, Rm+4 -> Rm")
  (code "0100nnnn10000110")

  (issue SH_DSP "1")
  (latency SH_DSP "1/5")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds.l	@Rm+,X1"
  SH_DSP
  (abstract "(Rm) -> X1, Rm+4 -> Rm")
  (code "0100nnnn10010110")

  (issue SH_DSP "1")
  (latency SH_DSP "1/5")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds.l	@Rm+,Y0"
  SH_DSP
  (abstract "(Rm) -> Y0, Rm+4 -> Rm")
  (code "0100nnnn10100110")

  (issue SH_DSP "1")
  (latency SH_DSP "1/5")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds.l	@Rm+,Y1"
  SH_DSP
  (abstract "(Rm) -> Y1, Rm+4 -> Rm")
  (code "0100nnnn10110110")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ldtbl"
  SH3 SH4 SH4A privileged
  (abstract "PTEH/PTEL -> TLB")
  (code "0000000000111000")

  (group SH4A "CO" SH4 "CO")
  (issue SH3 "1" SH4A "1" SH4 "1")
  (latency SH3 "1" SH4A "1" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movca.l	R0,@Rn"
  SH4 SH4A
  (abstract "R0 -> (Rn) (without fetching cache block)")
  (code "0000nnnn11000011")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH4 "1")
  (latency SH4A "1" SH4 "3-7")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "nop"
  SH_ANY
  (abstract "No operation")
  (code "0000000000001001")

  (group SH4A "MT" SH4 "MT")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "0" SH4 "0")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ocbi	@Rn"
  SH4 SH4A
  (abstract "Invalidate operand cache block")
  (code "0000nnnn10010011")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH4 "1")
  (latency SH4A "1" SH4 "1-2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ocbp	@Rn"
  SH4 SH4A
  (abstract "Write back and invalidate operand cache block")
  (code "0000nnnn10100011")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH4 "1")
  (latency SH4A "1" SH4 "1-5")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ocbwb	@Rn"
  SH4 SH4A
  (abstract "Write back operand cache block")
  (code "0000nnnn10110011")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH4 "1")
  (latency SH4A "1" SH4 "1-5")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "pref	@Rn"
  SH2A SH3 SH4 SH4A
  (abstract "(Rn) -> operand cache")
  (code "0000nnnn10000011")

  (group SH4A "LS" SH4 "LS")
  (issue SH3 "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH3 "1/2" SH4A "1" SH2A "0" SH4 "1")

  (description {R"(
     )"})

  (note {R"(
    <u>SH2A, SH3*</u><br/>
    The cache line size is 16 bytes. <br/><br/>

    <u>SH4*</u><br/>
    The cache line size is 32 bytes. )"})
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "prefi	@Rn"
  SH4A
  (abstract "Reads 32-byte instruction block into instruction cache")
  (code "0000nnnn11010011")

  (group SH4A "CO")
  (issue SH4A "13")
  (latency SH4A "10")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "resbank"
  SH2A
  (abstract "Bank -> R0 to R14, GBR, MACH, MACL, PR")
  (code "0000000001011011")

  (issue SH2A "9/19")
  (latency SH2A "8/20")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "rte"
  SH_ANY privileged
  (abstract "Delayed branch\nSH1*,SH2*: stack area -> PC/SR\nSH3*,SH4*: SSR/SPC -> SR/PC")
  (code "0000000000101011")

  (group SH4A "CO" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "5" SH2A "6" SH4 "5")
  (latency SH1 "4" SH2 "4" SH3 "4" SH4A "4" SH2A "5" SH4 "5")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "setrc	Rn"
  SH_DSP
  (abstract "Rn[11:0] -> RC (SR[27:16])")
  (code "0100mmmm00010100")

  (issue SH_DSP "1")
  (latency SH_DSP "3")

  // SH1-DSP latency: 1
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "setrc	#imm"
  SH_DSP
  (abstract "imm -> RC (SR[23:16]), 0 -> SR[27:24]")
  (code "10000010iiiiiiii")

  (issue SH_DSP "1")
  (latency SH_DSP "3")

  // SH1-DSP latency: 1
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sets"
  SH3 SH4 SH4A
  (abstract "1 -> T")
  (code "0000000000011000")

  (group SH4A "EX" SH4 "CO")
  (issue SH3 "1" SH4A "1" SH4 "1")
  (latency SH3 "1" SH4A "1" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sett"
  SH_ANY
  (abstract "1 -> T")
  (code "0000000000011000")
  (t_bit "1")

  (group SH4A "EX" SH4 "MT")
  (issue SH_ANY "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "0" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sleep"
  SH_ANY privileged
  (abstract "Sleep or standby")
  (code "0000000000011011")

  (group SH4A "CO" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "2" SH4A "ud" SH2A "5" SH4 "4")
  (latency SH1 "3" SH2 "3" SH3 "4" SH4A "ud" SH2A "0" SH4 "4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stbank	R0,@Rn"
  SH2A
  (abstract "R0 -> (specified register bank entry)")
  (code "0100nnnn11100001")

  (issue SH2A "7")
  (latency SH2A "6")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc	SR,Rn"
  SH_ANY privileged
  (abstract "SR -> Rn")
  (code "0000nnnn00000010")

  (group SH4A "CO" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "2")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc	GBR,Rn"
  SH_ANY
  (abstract "GBR -> Rn")
  (code "0000nnnn00010010")

  (group SH4A "LS" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "2")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc	VBR,Rn"
  SH_ANY privileged
  (abstract "VBR -> Rn")
  (code "0000nnnn00100010")

  (group SH4A "LS" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "2")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc	MOD,Rn"
  SH_DSP
  (abstract "MOD -> Rn")
  (code "0000nnnn01010010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc	RE,Rn"
  SH_DSP
  (abstract "RE -> Rn")
  (code "0000nnnn01110010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc	RS,Rn"
  SH_DSP
  (abstract "RS -> Rn")
  (code "0000nnnn01100010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc	SSR,Rn"
  SH3 SH4 SH4A privileged
  (abstract "SSR -> Rn")
  (code "0000nnnn00110010")

  (group SH4A "LS" SH4 "CO")
  (issue SH3 "1" SH4A "1" SH4 "2")
  (latency SH3 "1" SH4A "1" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc	SPC,Rn"
  SH3 SH4 SH4A privileged
  (abstract "SPC -> Rn")
  (code "0000nnnn01000010")

  (group SH4A "LS" SH4 "CO")
  (issue SH3 "1" SH4A "1" SH4 "2")
  (latency SH3 "1" SH4A "1" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc	SGR,Rn"
  SH4 SH4A privileged
  (abstract "SGR -> Rn")
  (code "0000nnnn00111010")

  (group SH4A "LS" SH4 "CO")
  (issue SH4A "1" SH4 "3")
  (latency SH4A "1" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc	DBR,Rn"
  SH4 SH4A privileged
  (abstract "DBR -> Rn")
  (code "0000nnnn11111010")

  (group SH4A "LS" SH4 "CO")
  (issue SH4A "1" SH4 "2")
  (latency SH4A "1" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc	Rm_BANK,Rn"
  SH3 SH4 SH4A privileged
  (abstract "Rm_BANK -> Rn (m = 0-7)")
  (code "0000nnnn1mmm0010")

  (group SH4A "LS" SH4 "CO")
  (issue SH3 "1" SH4A "1" SH4 "2")
  (latency SH3 "1" SH4A "1" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc.l	SR,@-Rn"
  SH_ANY privileged
  (abstract "Rn-4 -> Rn, SR -> (Rn)")
  (code "0100nnnn00000011")

  (group SH4A "CO" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "2")
  (latency SH1 "2" SH2 "2" SH3 "1/2" SH4A "1" SH2A "2" SH4 "2/2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc.l	GBR,@-Rn"
  SH_ANY
  (abstract "Rn-4 -> Rn, GBR -> (Rn)")
  (code "0100nnnn00010011")

  (group SH4A "LS" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "2")
  (latency SH1 "2" SH2 "2" SH3 "1/2" SH4A "1" SH2A "1" SH4 "2/2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc.l	VBR,@-Rn"
  SH_ANY privileged
  (abstract "Rn-4 -> Rn, VBR -> (Rn)")
  (code "0100nnnn00100011")

  (group SH4A "LS" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "2")
  (latency SH1 "2" SH2 "2" SH3 "1/2" SH4A "1" SH2A "1" SH4 "2/2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc.l	MOD,@-Rn"
  SH_DSP
  (abstract "Rn-4 -> Rn, MOD -> (Rn)")
  (code "0100nnnn01010011")

  (issue SH_DSP "1")
  (latency SH_DSP "1/2")

  // SH1-DSP latency: 2
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc.l	RE,@-Rn"
  SH_DSP
  (abstract "Rn-4 -> Rn, RE -> (Rn)")
  (code "0100nnnn01110011")

  (issue SH_DSP "1")
  (latency SH_DSP "1/2")

  // SH1-DSP latency: 2
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc.l	RS,@-Rn"
  SH_DSP
  (abstract "Rn-4 -> Rn, RS -> (Rn)")
  (code "0100nnnn01100011")

  (issue SH_DSP "1")
  (latency SH_DSP "1/2")

  // SH1-DSP latency: 2
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc.l	SSR,@-Rn"
  SH3 SH4 SH4A privileged
  (abstract "Rn-4 -> Rn, SSR -> (Rn)")
  (code "0100nnnn00110011")

  (group SH4A "LS" SH4 "CO")
  (issue SH3 "1" SH4A "1" SH4 "2")
  (latency SH3 "1/2" SH4A "1" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc.l	SPC,@-Rn"
  SH3 SH4 SH4A privileged
  (abstract "Rn-4 -> Rn, SPC -> (Rn)")
  (code "0100nnnn01000011")

  (group SH4A "LS" SH4 "CO")
  (issue SH4A "1" SH4 "2")
  (latency SH4A "1" SH4 "2/2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc.l	SGR,@-Rn"
  SH4 SH4A privileged
  (abstract "Rn-4 -> Rn, SGR -> (Rn)")
  (code "0100nnnn00110010")

  (group SH4A "LS" SH4 "CO")
  (issue SH4A "1" SH4 "3")
  (latency SH4A "1" SH4 "3/3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc.l	DBR,@-Rn"
  SH4 SH4A privileged
  (abstract "Rn-4 -> Rn, DBR -> (Rn)")
  (code "0100nnnn11110010")

  (group SH4A "LS" SH4 "CO")
  (issue SH4A "1" SH4 "2")
  (latency SH4A "1" SH4 "2/2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "stc.l	Rm_BANK,@-Rn"
  SH3 SH4 SH4A privileged
  (abstract "Rn-4 -> Rn, Rm_BANK -> (Rn) (m = 0-7)")
  (code "0100nnnn1mmm0011")

  (group SH4A "LS" SH4 "CO")
  (issue SH3 "2" SH4A "1" SH4 "2")
  (latency SH3 "2" SH4A "1" SH4 "2/2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts	MACH,Rn"
  SH_ANY
  (abstract "MACH -> Rn")
  (code "0000nnnn00001010")

  (group SH4A "LS" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts	MACL,Rn"
  SH_ANY
  (abstract "MACL -> Rn")
  (code "0000nnnn00011010")

  (group SH4A "LS" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "2" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts	PR,Rn"
  SH_ANY
  (abstract "PR -> Rn")
  (code "0000nnnn00101010")

  (group SH4A "LS" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "2")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts	DSR,Rn"
  SH_DSP
  (abstract "DSR -> Rn")
  (code "0000nnnn01101010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts	A0,Rn"
  SH_DSP
  (abstract "A0 -> Rn")
  (code "0000nnnn01111010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts	X0,Rn"
  SH_DSP
  (abstract "X0 -> Rn")
  (code "0000nnnn10001010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts	X1,Rn"
  SH_DSP
  (abstract "X1 -> Rn")
  (code "0000nnnn10011010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts	Y0,Rn"
  SH_DSP
  (abstract "Y0 -> Rn")
  (code "0000nnnn10101010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts	Y1,Rn"
  SH_DSP
  (abstract "Y1 -> Rn")
  (code "0000nnnn10111010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts.l	MACH,@-Rn"
  SH_ANY
  (abstract "Rn-4 -> Rn, MACH -> (Rn)")
  (code "0100nnnn00000010")

  (group SH4A "LS" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "1/1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts.l	MACL,@-Rn"
  SH_ANY
  (abstract "Rn-4 -> Rn, MACL -> (Rn)")
  (code "0100nnnn00010010")

  (group SH4A "LS" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "1/1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts.l	PR,@-Rn"
  SH_ANY
  (abstract "Rn-4 -> Rn, PR -> (Rn)")
  (code "0100nnnn00100010")

  (group SH4A "LS" SH4 "CO")
  (issue SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "2")
  (latency SH1 "1" SH2 "1" SH3 "1" SH4A "1" SH2A "1" SH4 "2/2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts.l	DSR,@-Rn"
  SH_DSP
  (abstract "Rn-4 -> Rn, DSR -> (Rn)")
  (code "0100nnnn01100010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts.l	A0,@-Rn"
  SH_DSP
  (abstract "Rn-4 -> Rn, A0 -> (Rn)")
  (code "0100nnnn01100010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts.l	X0,@-Rn"
  SH_DSP
  (abstract "Rn-4 -> Rn, X0 -> (Rn)")
  (code "0100nnnn10000010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts.l	X1,@-Rn"
  SH_DSP
  (abstract "Rn-4 -> Rn, X1 -> (Rn)")
  (code "0100nnnn10010010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts.l	Y0,@-Rn"
  SH_DSP
  (abstract "Rn-4 -> Rn, Y0 -> (Rn)")
  (code "0100nnnn10100010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts.l	Y1,@-Rn"
  SH_DSP
  (abstract "Rn-4 -> Rn, Y1 -> (Rn)")
  (code "0100nnnn10110010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "synco"
  SH4A
  (abstract "Prevents the next instruction from being issued until instructions issued before this instruction has been completed.")
  (code "0000000010101011")

  (group SH4A "CO")
  (issue SH4A "ud")
  (latency SH4A "ud")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "trapa	#imm"
  SH_ANY
  (abstract {R"(SH1*,SH2*: PC/SR -> stack area, (imm*4 + VBR) -> PC
SH3*,SH4*: PC/SR -> SPC/SSR, imm*4 -> TRA, 0x160 -> EXPEVT, VBR + 0x0100 -> PC)"})
  (code "11000011iiiiiiii")

  (group SH4A "CO" SH4 "CO")
  (issue SH1 "2" SH2 "2" SH3 "2" SH4A "14" SH2A "5" SH4 "7")
  (latency SH1 "8" SH2 "8" SH3 "8" SH4A "13" SH2A "6" SH4 "7")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
));



__sexpr (insn_blocks.push_back
(insns "32 Bit Floating-Point Data Transfer Instructions (FPSCR.SZ = 0)"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov	FRm,FRn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "FRm -> FRn")
  (code "1111nnnnmmmm1100")

  (group SH4A "LS" SH4 "LS")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "0" SH4 "0")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov.s	@Rm,FRn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "(Rm) -> FRn")
  (code "1111nnnnmmmm1000")

  (group SH4A "LS" SH4 "LS")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "0/2" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov.s	@Rm+,FRn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "(Rm) -> FRn, Rm+4 -> Rm")
  (code "1111nnnnmmmm1001")

  (group SH4A "LS" SH4 "LS")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "1/2" SH4 "1/2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov.s	@(R0,Rm),FRn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "(R0 + Rm) -> FRn")
  (code "1111nnnnmmmm0110")

  (group SH4A "LS" SH4 "LS")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "0/2" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov.s	@(disp12,Rm),FRn"
  SH2A
  (abstract "(disp*4 + Rm) -> FRn")
  (code "0011nnnnmmmm0001 0111dddddddddddd")

  (issue SH2A "1")
  (latency SH2A "0/2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov.s	FRm,@Rn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "FRm -> (Rn)")
  (code "1111nnnnmmmm1010")

  (group SH4A "LS" SH4 "LS")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "0" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov.s	FRm,@-Rn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "Rn-4 -> Rn, FRm -> (Rn)")
  (code "1111nnnnmmmm1011")

  (group SH4A "LS" SH4 "LS")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "1/0" SH4 "1/1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov.s	FRm,@(R0,Rn)"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "FRm -> (R0 + Rn)")
  (code "1111nnnnmmmm0111")

  (group SH4A "LS" SH4 "LS")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "0" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov.s	FRm,@(disp12,Rn)"
  SH2A
  (abstract "FRm -> (disp*4 + Rn)")
  (code "0011nnnnmmmm0001 0011dddddddddddd")

  (issue SH2A "1")
  (latency SH2A "0")
)

));



__sexpr (insn_blocks.push_back
(insns "64 Bit Floating-Point Data Transfer Instructions (FPSCR.SZ = 1)"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov.d	DRm,DRn"
  SH4 SH4A SH2A
  (abstract "DRm -> DRn")
  (code "1111nnn0mmm01100")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH2A "2" SH4 "1")
  (latency SH4A "1" SH2A "1" SH4 "0")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov.d	@Rm,DRn"
  SH4 SH4A SH2A
  (abstract "(Rm) -> DRn")
  (code "1111nnn0mmmm1000")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH2A "2" SH4 "1")
  (latency SH4A "1" SH2A "0/4" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov.d	@Rm+,DRn"
  SH4 SH4A SH2A
  (abstract "(Rm) -> DRn, Rm + 8 -> Rm")
  (code "1111nnn0mmmm1001")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH2A "2" SH4 "1")
  (latency SH4A "1" SH2A "1/4" SH4 "1/2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov.d	@(R0,Rm),DRn"
  SH4 SH4A SH2A
  (abstract "(R0 + Rm) -> DRn")
  (code "1111nnn0mmmm0110")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH2A "2" SH4 "1")
  (latency SH4A "1" SH2A "0/4" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov.d	@(disp12,Rm),DRn"
  SH2A
  (abstract "(disp*8 + Rm) -> DRn")
  (code "0011nnn0mmmm0001 0111dddddddddddd")

  (issue SH2A "2")
  (latency SH2A "0/4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov.d	DRm,@Rn"
  SH4 SH4A SH2A
  (abstract "DRm -> (Rn)")
  (code "1111nnnnmmm01010")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH2A "2" SH4 "1")
  (latency SH4A "1" SH2A "0" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov.d	DRm,@-Rn"
  SH4 SH4A SH2A
  (abstract "Rn-8 -> Rn, DRm -> (Rn)")
  (code "1111nnnnmmm01011")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH2A "2" SH4 "1")
  (latency SH4A "1" SH2A "0/1" SH4 "1/1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov.d	DRm,@(R0,Rn)"
  SH4 SH4A SH2A
  (abstract "DRm -> (R0 + Rn)")
  (code "1111nnnnmmm00111")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH2A "2" SH4 "1")
  (latency SH4A "1" SH2A "0" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov.d	DRm,@(disp12,Rn)"
  SH2A
  (abstract "DRm -> (disp*8 + Rn)")
  (code "0011nnnnmmm00001 0011dddddddddddd")

  (issue SH2A "2")
  (latency SH2A "0")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov	DRm,XDn"
  SH4 SH4A
  (abstract "DRm -> XDn")
  (code "1111nnn1mmm01100")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH4 "1")
  (latency SH4A "1" SH4 "0")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov	XDm,DRn"
  SH4 SH4A
  (abstract "XDm -> DRn")
  (code "1111nnn0mmm11100")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH4 "1")
  (latency SH4A "1" SH4 "0")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov	XDm,XDn"
  SH4 SH4A
  (abstract "XDm -> XDn")
  (code "1111nnn1mmm11100")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH4 "1")
  (latency SH4A "1" SH4 "0")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov	@Rm,XDn"
  SH4 SH4A
  (abstract "(Rm) -> XDn")
  (code "1111nnn1mmmm1000")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH4 "1")
  (latency SH4A "1" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov	@Rm+,XDn"
  SH4 SH4A
  (abstract "(Rm) -> XDn, Rm+8 -> Rm")
  (code "1111nnn1mmmm1001")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH4 "1")
  (latency SH4A "1" SH4 "1/2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov	@(R0,Rm),XDn"
  SH4 SH4A
  (abstract "(R0 + Rm) -> XDn")
  (code "1111nnn1mmmm0110")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH4 "1")
  (latency SH4A "1" SH4 "2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov	XDm,@Rn"
  SH4 SH4A
  (abstract "XDm -> (Rn)")
  (code "1111nnnnmmm11010")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH4 "1")
  (latency SH4A "1" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov	XDm,@-Rn"
  SH4 SH4A
  (abstract "Rn-8 -> Rn, (Rn) -> XDm")
  (code "1111nnnnmmm11011")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH4 "1")
  (latency SH4A "1" SH4 "1/1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmov	XDm,@(R0 + Rn)"
  SH4 SH4A
  (abstract "XDm -> (R0 + Rn)")
  (code "1111nnnnmmm10111")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH4 "1")
  (latency SH4A "1" SH4 "1")
)


));



__sexpr (insn_blocks.push_back
(insns "Floating-Point Single-Precision Instructions (FPSCR.PR = 0)"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fldi0	FRn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "0x00000000 -> FRn")
  (code "1111nnnn10001101")

  (group SH4A "LS" SH4 "LS")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "0" SH4 "0")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fldi1	FRn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "0x3F800000 -> FRn")
  (code "1111nnnn10011101")

  (group SH4A "LS" SH4 "LS")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "0" SH4 "0")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "flds	FRm,FPUL"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "FRm -> FPUL")
  (code "1111mmmm00011101")

  (group SH4A "LS" SH4 "LS")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "0" SH4 "0")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fsts	FPUL,FRn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "FPUL -> FRn")
  (code "1111nnnn00001101")

  (group SH4A "LS" SH4 "LS")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "0" SH4 "0")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fabs	FRn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "FRn & 0x7FFFFFFF -> FRn")
  (code "1111nnnn01011101")

  (group SH4A "LS" SH4 "LS")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "0" SH4 "0")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fadd	FRm,FRn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "FRn + FRm -> FRn")
  (code "1111nnnnmmmm0000")

  (group SH4A "FE" SH4 "FE")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "3" SH4 "3/4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fcmp/eq	FRm,FRn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "If FRn = FRm: 1 -> T, else: 0 -> T")
  (code "1111nnnnmmmm0100")
  (t_bit "Result")

  (group SH4A "FE" SH4 "FE")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "2" SH4 "2/4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fcmp/gt	FRm,FRn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "If FRn > FRm: 1 -> T, else: 0 -> T")
  (code "1111nnnnmmmm0101")
  (t_bit "Result")

  (group SH4A "FE" SH4 "FE")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "2" SH4 "2/4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fdiv	FRm,FRn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "FRn / FRm -> FRn")
  (code "1111nnnnmmmm0011")

  (group SH4A "FE" SH4 "FE")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "13" SH3E "13" SH4A "14" SH2A "12" SH4 "12/13")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "float	FPUL,FRn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "(float)FPUL -> FRn")
  (code "1111nnnn00101101")

  (group SH4A "FE" SH4 "FE")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "3" SH4 "3/4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmac	FR0,FRm,FRn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "FR0 * FRm + FRn -> FRn")
  (code "1111nnnnmmmm1110")

  (group SH4A "FE" SH4 "FE")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "3" SH4 "3/4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmul	FRm,FRn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "FRn * FRm -> FRn")
  (code "1111nnnnmmmm0010")

  (group SH4A "FE" SH4 "FE")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "3" SH4 "3/4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fneg	FRn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "FRn ^ 0x80000000 -> FRn")
  (code "1111nnnn01001101")

  (group SH4A "LS" SH4 "LS")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "0" SH4 "0")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fsqrt	FRn"
  SH3E SH4 SH4A SH2A
  (abstract "sqrt (FRn) -> FRn")
  (code "1111nnnn01101101")

  (group SH4A "FE" SH4 "FE")
  (issue SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH3E "13" SH4A "30" SH2A "11" SH4 "11/12")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fsub	FRm,FRn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "FRn - FRm -> FRn")
  (code "1111nnnnmmmm0001")

  (group SH4A "FE" SH4 "FE")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "3" SH4 "3/4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ftrc	FRm,FPUL"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "(long)FRm -> FPUL")
  (code "1111mmmm00111101")

  (group SH4A "FE" SH4 "FE")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "3" SH4 "3/4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fipr	FVm,FVn"
  SH4 SH4A
  (abstract "inner_product (FVm, FVn) -> FR[n+3]")
  (code "1111nnmm11101101")

  (group SH4A "FE" SH4 "FE")
  (issue SH4A "1" SH4 "1")
  (latency SH4A "1" SH4 "4/5")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ftrv	XMTRX,FVn"
  SH4 SH4A
  (abstract "transform_vector (XMTRX, FVn) -> FVn")
  (code "1111nn0111111101")

  (group SH4A "FE" SH4 "FE")
  (issue SH4A "1" SH4 "1")
  (latency SH4A "4" SH4 "5/8")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fsrra	FRn"
  SH4A
  (abstract "1.0 / sqrt (FRn) -> FRn")
  (code "1111nnnn01111101")

  (group SH4A "FE")
  (issue SH4A "1")
  (latency SH4A "1")

  (note {R"(
    This instruction is also supported by the SH7091 (SH4). )"})
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fsca	FPUL,DRn"
  SH4A
  (abstract "sin(FPUL) -> FRn\ncos(FPUL) -> FR[n+1]")
  (code "1111nnn011111101")

  (group SH4A "FE")
  (issue SH4A "1")
  (latency SH4A "3")

  (note {R"(
    This instruction is also supported by the SH7091 (SH4). )"})
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
));


__sexpr (insn_blocks.push_back
(insns "Floating-Point Double-Precision Instructions (FPSCR.PR = 1)"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fabs	DRn"
  SH4 SH4A SH2A
  (abstract "DRn & 0x7FFFFFFFFFFFFFFF -> DRn")
  (code "1111nnn001011101")

  (group SH4A "LS" SH4 "LS")
  (issue SH4A "1" SH2A "1" SH4 "1")
  (latency SH4A "1" SH2A "0" SH4 "0")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fadd	DRm,DRn"
  SH4 SH4A SH2A
  (abstract "DRn + DRm -> DRn")
  (code "1111nnn0mmm00000")

  (group SH4A "FE" SH4 "FE")
  (issue SH4A "1" SH2A "1" SH4 "1")
  (latency SH4A "1" SH2A "0/8" SH4 "7/9")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fcmp/eq	DRm,DRn"
  SH4 SH4A SH2A
  (abstract "If DRn = DRm: 1 -> T, else: 0 -> T")
  (code "1111nnn0mmm00100")

  (group SH4A "FE" SH4 "CO")
  (issue SH4A "1" SH2A "2" SH4 "2")
  (latency SH4A "1" SH2A "3" SH4 "3/5")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fcmp/gt	DRm,DRn"
  SH4 SH4A SH2A
  (abstract "If DRn > DRm: 1 -> T, else: 0 -> T")
  (code "1111nnn0mmm00101")

  (group SH4A "FE" SH4 "CO")
  (issue SH4A "1" SH2A "2" SH4 "2")
  (latency SH4A "1" SH2A "3" SH4 "3/5")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fcnvds	DRm,FPUL"
  SH4 SH4A SH2A
  (abstract "double_to_float (DRm) -> FPUL")
  (code "1111mmm010111101")

  (group SH4A "FE" SH4 "FE")
  (issue SH4A "1" SH2A "1" SH4 "1")
  (latency SH4A "1" SH2A "4" SH4 "4/5")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fcnvsd	FPUL,DRn"
  SH4 SH4A SH2A
  (abstract "float_to_double (FPUL) -> DRn")
  (code "1111nnn010101101")

  (group SH4A "FE" SH4 "FE")
  (issue SH4A "1" SH2A "1" SH4 "1")
  (latency SH4A "1" SH2A "4" SH4 "3/5")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fdiv	DRm,DRn"
  SH4 SH4A SH2A
  (abstract "DRn / DRm -> DRn")
  (code "1111nnn0mmm00011")

  (group SH4A "FE" SH4 "FE")
  (issue SH4A "1" SH2A "1" SH4 "1")
  (latency SH4A "14" SH2A "0/24" SH4 "24/26")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "float	FPUL,DRn"
  SH4 SH4A SH2A
  (abstract "(double)FPUL -> DRn")
  (code "1111nnn000101101")

  (group SH4A "FE" SH4 "FE")
  (issue SH4A "1" SH2A "1" SH4 "1")
  (latency SH4A "1" SH2A "0/4" SH4 "3/5")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fmul	DRm,DRn"
  SH4 SH4A SH2A
  (abstract "DRn * DRm -> DRn")
  (code "1111nnn0mmm00010")

  (group SH4A "FE" SH4 "FE")
  (issue SH4A "1" SH2A "1" SH4 "1")
  (latency SH4A "3" SH2A "0/8" SH4 "7/9")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fneg	DRn"
  SH4 SH4A SH2A
  (abstract "DRn ^ 0x8000000000000000 -> DRn")
  (code "1111nnn001001101")

  (group SH4A "LS" SH4 "FS")
  (issue SH4A "1" SH2A "1" SH4 "1")
  (latency SH4A "1" SH2A "0" SH4 "0")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fsqrt	DRn"
  SH4 SH4A SH2A
  (abstract "sqrt (DRn) -> DRn")
  (code "1111nnn001101101")

  (group SH4A "FE" SH4 "FE")
  (issue SH4A "1" SH2A "1" SH4 "1")
  (latency SH4A "30" SH2A "0/24" SH4 "23/25")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fsub	DRm,DRn"
  SH4 SH4A SH2A
  (abstract "DRn - DRm -> DRn")
  (code "1111nnn0mmm00001")

  (group SH4A "FE" SH4 "FE")
  (issue SH4A "1" SH2A "1" SH4 "1")
  (latency SH4A "1" SH2A "0/8" SH4 "7/9")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "ftrc	DRm,FPUL"
  SH4 SH4A SH2A
  (abstract "(long)DRm -> FPUL")
  (code "1111mmm000111101")

  (group SH4A "FE" SH4 "FE")
  (issue SH4A "1" SH2A "1" SH4 "1")
  (latency SH4A "1" SH2A "0/4" SH4 "4/5")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
));


__sexpr (insn_blocks.push_back
(insns "Floating-Point Control Instructions"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds	Rm,FPSCR"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "Rm -> FPSCR")
  (code "0100mmmm01101010")

  (group SH4A "LS" SH4 "CO")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "3" SH4 "4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds	Rm,FPUL"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "Rm -> FPUL")
  (code "0100mmmm01011010")

  (group SH4A "LS" SH4 "LS")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds.l	@Rm+,FPSCR"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "(Rm) -> FPSCR, Rm+4 -> Rm")
  (code "0100mmmm01100110")

  (group SH4A "LS" SH4 "CO")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "3" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "lds.l	@Rm+,FPUL"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "(Rm) -> FPUL, Rm+4 -> Rm")
  (code "0100mmmm01010110")

  (group SH4A "LS" SH4 "LS")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "2" SH4 "1/2")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts	FPSCR,Rn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "FPSCR -> Rn")
  (code "0000nnnn01101010")

  (group SH4A "LS" SH4 "CO")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "2" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts	FPUL,Rn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "FPUL -> Rn")
  (code "0000nnnn01011010")

  (group SH4A "LS" SH4 "LS")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "2" SH4 "3")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts.l	FPSCR,@-Rn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "Rn-4 -> Rn, FPSCR -> (Rn)")
  (code "0100nnnn01100010")

  (group SH4A "LS" SH4 "CO")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1/1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "sts.l	FPUL,@-Rn"
  SH2E SH3E SH4 SH4A SH2A
  (abstract "Rn-4 -> Rn, FPUL -> (Rn)")
  (code "0100nnnn01010010")

  (group SH4A "LS" SH4 "CO")
  (issue SH2E "1" SH3E "1" SH4A "1" SH2A "1" SH4 "1")
  (latency SH2E "1" SH3E "1" SH4A "1" SH2A "2" SH4 "1/1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "frchg"
  SH4 SH4A
  (abstract "If FPSCR.PR = 0: ~FPSCR.FR -> FPSCR.FR\nElse: Undefined Operation")
  (code "1111101111111101")

  (group SH4A "FE" SH4 "FE")
  (issue SH4A "1" SH4 "1")
  (latency SH4A "1" SH4 "1/4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fschg"
  SH2A SH4 SH4A
  (abstract "If FPSCR.PR = 0: ~FPSCR.SZ -> FPSCR.SZ\nElse: Undefined Operation")
  (code "1111001111111101")

  (group SH4A "FE" SH4 "FE")
  (issue SH4A "1" SH2A "1" SH4 "1")
  (latency SH4A "1" SH2A "1" SH4 "1/4")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "fpchg"
  SH4A
  (abstract "~FPSCR.PR -> FPSCR.PR")
  (code "1111011111111101")

  (group SH4A "FE")
  (issue SH4A "1")
  (latency SH4A "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
));



__sexpr (insn_blocks.push_back
(insns "DSP Data Transfer Instructions"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "nopx"
  SH_DSP
  (abstract "No operation")
  (code "1111000*0*0*00**")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movx.w	@Ax,Dx"
  SH_DSP
  (abstract "(Ax) -> MSW of Dx, 0 -> LSW of Dx")
  (code "111100A*D*0*01**")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movx.w	@Ax+,Dx"
  SH_DSP
  (abstract "(Ax) -> MSW of Dx, 0 -> LSW of Dx, Ax+2 -> Ax")
  (code "111100A*D*0*10**")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movx.w	@Ax+Ix,Dx"
  SH_DSP
  (abstract "(Ax) -> MSW of Dx, 0 -> LSW of Dx, Ax+Ix -> Ax")
  (code "111100A*D*0*11**")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movx.w	Da,@Ax+"
  SH_DSP
  (abstract "MSW of Da -> (Ax)")
  (code "111100A*D*1*01**")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movx.w	Da,@Ax+"
  SH_DSP
  (abstract "MSW of Da -> (Ax), Ax+2 -> Ax")
  (code "111100A*D*1*10**")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movx.w	Da,@Ax+Ix"
  SH_DSP
  (abstract "MSW of Da -> (Ax), Ax+Ix -> Ax")
  (code "111100A*D*1*11**")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "nopy"
  SH_DSP
  (abstract "No Operation")
  (code "111100*0*0*0**00")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movy.w	@Ay,Dy"
  SH_DSP
  (abstract "(Ay) -> MSW of Dy, 0 -> LSW of Dy")
  (code "111100*A*D*0**01")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movy.w	@Ay+,Dy"
  SH_DSP
  (abstract "(Ay) -> MSW of Dy, 0 -> LSW of Dy, Ay+2 -> Ay")
  (code "111100*A*D*0**10")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movy.w	@Ay+Iy,Dy"
  SH_DSP
  (abstract "(Ay) -> MSW of Dy, 0 -> LSW of Dy, Ay+Iy -> Ay")
  (code "111100*A*D*0**11")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movy.w	Da,@Ay"
  SH_DSP
  (abstract "MSW of Da -> (Ay)")
  (code "111100*A*D*1**01")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movy.w	Da,@Ay+"
  SH_DSP
  (abstract "MSW of Da -> (Ay), Ay+2 -> Ay")
  (code "111100*A*D*1**10")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movy.w	Da,@Ay+Iy"
  SH_DSP
  (abstract "MSW of Da -> (Ay), Ay+Iy -> Ay")
  (code "111100*A*D*1**11")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movs.w	@-As,Ds"
  SH_DSP
  (abstract "As-2 -> As, (As) -> MSW of Ds, 0 -> LSW of Ds")
  (code "111101AADDDD0000")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movs.w	@As,Ds"
  SH_DSP
  (abstract "(As) -> MSW of Ds, 0 -> LSW of Ds")
  (code "111101AADDDD0100")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movs.w	@As+,Ds"
  SH_DSP
  (abstract "(As) -> MSW of Ds, 0 -> LSW of Ds, As+2 -> As")
  (code "111101AADDDD1000")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movs.w	@As+Ix,Ds"
  SH_DSP
  (abstract "(As) -> MSW of Ds, 0 -> LSW of DS, As+Ix -> As")
  (code "111101AADDDD1100")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movs.w	Ds,@-As"
  SH_DSP
  (abstract "As-2 -> As, MSW of Ds -> (As)")
  (code "111101AADDDD0001")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movs.w	Ds,@As"
  SH_DSP
  (abstract "MSW of Ds -> (As)")
  (code "111101AADDDD0101")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movs.w	Ds,@As+"
  SH_DSP
  (abstract "MSW of Ds -> (As), As+2 -> As")
  (code "111101AADDDD1001")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movs.w	Ds,@As+Is"
  SH_DSP
  (abstract "MSW of DS -> (As), As+Is -> As")
  (code "111101AADDDD1101")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movs.l	@-As,Ds"
  SH_DSP
  (abstract "As-4 -> As, (As) -> Ds")
  (code "111101AADDDD0010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movs.l	@As,Ds"
  SH_DSP
  (abstract "(As) -> Ds")
  (code "111101AADDDD0110")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movs.l	@As+,Ds"
  SH_DSP
  (abstract "(As) -> Ds, As+4 -> As")
  (code "111101AADDDD1010")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movs.l	@As+Is,Ds"
  SH_DSP
  (abstract "(As) -> Ds, As+Is -> As")
  (code "111101AADDDD1110")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movs.l	Ds,@-As"
  SH_DSP
  (abstract "As-4 -> As, Ds -> (As)")
  (code "111101AADDDD0011")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movs.l	Ds,@As"
  SH_DSP
  (abstract "Ds -> (As)")
  (code "111101AADDDD0111")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movs.l	Ds,@As+"
  SH_DSP
  (abstract "Ds -> (As), As+4 -> As")
  (code "111101AADDDD1011")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "movs.l	Ds,@As+Is"
  SH_DSP
  (abstract "Ds -> (As), As+Is -> As")
  (code "111101AADDDD1111")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
));


__sexpr (insn_blocks.push_back
(insns "DSP ALU Arithmetic Operation Instructions"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "pabs		Sx,Dz"
  SH_DSP
  (abstract "If Sx >= 0: Sx -> Dz\nIf Sx < 0: 0 - Sx -> Dz")
  (code "111110********** 10001000xx00zzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "pabs		Sy,Dz"
  SH_DSP
  (abstract "If Sy >= 0: Sy -> Dz\nIf Sy < 0: 0 - Sy -> Dz")
  (code "111110********** 1010100000yyzzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "padd		Sx,Sy,Dz"
  SH_DSP
  (abstract "Sx + Sy -> Dz")
  (code "111110********** 10110001xxyyzzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct padd	Sx,Sy,Dz"
  SH_DSP
  (abstract "If DC = 1: Sx + Sy -> Dz, else: nop")
  (code "111110********** 10110010xxyyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf padd	Sx,Sy,Dz"
  SH_DSP
  (abstract "If DC = 0: Sx + Sy -> Dz, else: nop")
  (code "111110********** 10110011xxyyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "padd		Sx,Sy,Du\npmuls		Se,Sf,Dg"
  SH_DSP
  (abstract "Sx + Sy -> Du\nMSW of Se * MSW of Sf -> Dg")
  (code "111110********** 0111eeffxxyygguu")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "paddc		Sx,Sy,Dz"
  SH_DSP
  (abstract "Sx + Sy + DC -> Dz")
  (code "111110********** 10110000xxyyzzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "pclr		Dz"
  SH_DSP
  (abstract "0x00000000 -> Dz")
  (code "111110********** 100011010000zzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct pclr	Dz"
  SH_DSP
  (abstract "If DC = 1: 0x00000000 -> Dz, else: nop")
  (code "111110********** 100011100000zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf pclr	Dz"
  SH_DSP
  (abstract "If DC = 0: 0x00000000 -> Dz, else: nop")
  (code "111110********** 100011110000zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "pcmp		Sx,Sy"
  SH_DSP
  (abstract "Sx - Sy")
  (code "111110********** 10000100xxyy0000")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "pcopy		Sx,Dz"
  SH_DSP
  (abstract "Sx -> Dz")
  (code "111110********** 11011001xx00zzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "pcopy		Sy,Dz"
  SH_DSP
  (abstract "Sy -> Dz")
  (code "111110********** 1111100100yyzzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct pcopy	Sx,Dz"
  SH_DSP
  (abstract "If DC = 1: Sx -> Dz, else: nop")
  (code "111110********** 11011010xx00zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct pcopy	Sy,Dz"
  SH_DSP
  (abstract "If DC = 1: Sy -> Dz, else: nop")
  (code "111110********** 1111101000yyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf pcopy	Sx,Dz"
  SH_DSP
  (abstract "If DC = 0: Sx -> Dz, else: nop")
  (code "111110********** 11011011xx00zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf pcopy	Sy,Dz"
  SH_DSP
  (abstract "If DC = 0: Sy -> Dz, else: nop")
  (code "111110********** 1111101100yyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "pneg		Sx,Dz"
  SH_DSP
  (abstract "0 - Sx -> Dz")
  (code "111110********** 11001001xx00zzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "pneg		Sy,Dz"
  SH_DSP
  (abstract "0 - Sy -> Dz")
  (code "111110********** 1110100100yyzzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct pneg	Sx,Dz"
  SH_DSP
  (abstract "If DC = 1: 0 - Sx -> Dz, else: nop")
  (code "111110********** 11001010xx00zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct pneg	Sy,Dz"
  SH_DSP
  (abstract "If DC = 1: 0 - Sy -> Dz, else: nop")
  (code "111110********** 1110101000yyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf pneg	Sx,Dz"
  SH_DSP
  (abstract "If DC = 0: 0 - Sx -> Dz, else: nop")
  (code "111110********** 11001011xx00zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf pneg	Sy,Dz"
  SH_DSP
  (abstract "If DC = 0: 0 - Sy -> Dz, else: nop")
  (code "111110********** 1110101100yyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "psub		Sx,Sy,Dz"
  SH_DSP
  (abstract "Sx - Sy -> Dz")
  (code "111110********** 10100001xxyyzzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct psub	Sx,Sy,Dz"
  SH_DSP
  (abstract "If DC = 1: Sx - Sy -> Dz, else: nop")
  (code "111110********** 10100010xxyyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf psub 	Sx,Sy,Dz"
  SH_DSP
  (abstract "If DC = 0: Sx - Sy -> Dz, else: nop")
  (code "111110********** 10100011xxyyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "psub		Sx,Sy,Du\npmuls		Se,Sf,Dg"
  SH_DSP
  (abstract "Sx - Sy -> Du\nMSW of Se * MSW of Sf -> Dg")
  (code "111110********** 0110eeffxxyygguu")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "psubc		Sx,Sy,Dz"
  SH_DSP
  (abstract "Sx - Sy - DC -> Dz")
  (code "111110********** 10100000xxyyzzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "pdec		Sx,Dz"
  SH_DSP
  (abstract "MSW of Sx - 1 -> MSW of Dz, clear LSW of Dz")
  (code "111110********** 10001001xx00zzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "pdec		Sy,Dz"
  SH_DSP
  (abstract "MSW of Sy - 1 -> MSW of Dz, clear LSW of Dz")
  (code "111110********** 1010100100yyzzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct pdec	Sx,Dz"
  SH_DSP
  (abstract "If DC = 1: MSW of Sx - 1 -> MSW of DZ, clear LSW of Dz; else: nop")
  (code "111110********** 10001010xx00zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct pdec	Sy,Dz"
  SH_DSP
  (abstract "If DC = 1: MSW of Sy - 1 -> MSW of DZ, clear LSW of Dz; else: nop")
  (code "111110********** 1010101000yyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf pdec	Sx,Dz"
  SH_DSP
  (abstract "If DC = 0: MSW of Sx - 1 -> MSW of DZ, clear LSW of Dz; else: nop")
  (code "111110********** 10001011xx00zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf pdec	Sy,Dz"
  SH_DSP
  (abstract "If DC = 0: MSW of Sy - 1 -> MSW of DZ, clear LSW of Dz; else: nop")
  (code "111110********** 1010101100yyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "pinc		Sx,Dz"
  SH_DSP
  (abstract "MSW of Sy + 1 -> MSW of Dz, clear LSW of Dz")
  (code "111110********** 10011001xx00zzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "pinc		Sy,Dz"
  SH_DSP
  (abstract "MSW of Sy + 1 -> MSW of Dz, clear LSW of Dz")
  (code "111110********** 1011100100yyzzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct pinc	Sx,Dz"
  SH_DSP
  (abstract "If DC = 1: MSW of Sx + 1 -> MSW of Dz, clear LSW of Dz; else: nop")
  (code "111110********** 10011010xx00zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct pinc	Sy,Dz"
  SH_DSP
  (abstract "If DC = 1: MSW of Sy + 1 -> MSW of Dz, clear LSW of Dz; else: nop")
  (code "111110********** 1011101000yyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf pinc	Sx,Dz"
  SH_DSP
  (abstract "If DC = 0: MSW of Sx + 1 -> MSW of Dz, clear LSW of Dz; else: nop")
  (code "111110********** 10011011xx00zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf pinc	Sy,Dz"
  SH_DSP
  (abstract "If DC = 0: MSW of Sy + 1 -> MSW of Dz, clear LSW of Dz; else: nop")
  (code "111110********** 1011101100yyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "pdmsb		Sx,Dz"
  SH_DSP
  (abstract "Sx data MSB position -> MSW of Dz, clear LSW of Dz")
  (code "111110********** 10011101xx00zzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "pdmsb		Sy,Dz"
  SH_DSP
  (abstract "Sy data MSB position -> MSW of Dz, clear LSW of Dz")
  (code "111110********** 1011110100yyzzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct pdmsb	Sx,Dz"
  SH_DSP
  (abstract "If DC = 1: Sx data MSB position -> MSW of Dz, clear LSW of Dz; else: nop")
  (code "111110********** 10011110xx00zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct pdmsb	Sy,Dz"
  SH_DSP
  (abstract "If DC = 1: Sy data MSB position -> MSW of Dz, clear LSW of Dz; else: nop")
  (code "111110********** 1011111000yyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf pdmsb	Sx,Dz"
  SH_DSP
  (abstract "If DC = 0: Sx data MSB position -> MSW of Dz, clear LSW of Dz; else: nop")
  (code "111110********** 10011111xx00zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf pdmsb	Sy,Dz"
  SH_DSP
  (abstract "If DC = 0: Sy data MSB position -> MSW of Dz, clear LSW of Dz; else: nop")
  (code "111110********** 1011111100yyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "prnd		Sx,Dz"
  SH_DSP
  (abstract "Sx + 0x00008000 -> Dz, clear LSW of Dz")
  (code "111110********** 10011000xx00zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "prnd		Sy,Dz"
  SH_DSP
  (abstract "Sy + 0x00008000 -> Dz, clear LSW of Dz")
  (code "111110********** 1011100000yyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
));


__sexpr (insn_blocks.push_back
(insns "DSP ALU Logical Operation Instructions"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "pand		Sx,Sy,Dz"
  SH_DSP
  (abstract "Sx & Sy -> Dz, clear LSW of Dz")
  (code "111110********** 10010101xxyyzzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct pand	Sx,Sy,Dz"
  SH_DSP
  (abstract "If DC = 1: Sx & Sy -> Dz, clear LSW of Dz; else: nop")
  (code "111110********** 10010110xxyyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf pand	Sx,Sy,Dz"
  SH_DSP
  (abstract "If DC = 0: Sx & Sy -> Dz, clear LSW of Dz; else: nop")
  (code "111110********** 10010111xxyyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "por		Sx,Sy,Dz"
  SH_DSP
  (abstract "Sx | Sy -> Dz, clear LSW of Dz")
  (code "111110********** 10110101xxyyzzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct por		Sx,Sy,Dz"
  SH_DSP
  (abstract "If DC = 1: Sx | Sy -> Dz, clear LSW of Dz; else: nop")
  (code "111110********** 10110110xxyyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf por		Sx,Sy,Dz"
  SH_DSP
  (abstract "If DC = 0: Sx | Sy -> Dz, clear LSW of Dz; else: nop")
  (code "111110********** 10110111xxyyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "pxor		Sx,Sy,Dz"
  SH_DSP
  (abstract "Sx ^ Sy -> Dz, clear LSW of Dz")
  (code "111110********** 10100101xxyyzzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct pxor	Sx,Sy,Dz"
  SH_DSP
  (abstract "If DC = 1: Sx ^ Sy -> Dz, clear LSW of Dz; else: nop")
  (code "111110********** 10100110xxyyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf pxor	Sx,Sy,Dz"
  SH_DSP
  (abstract "If DC = 0: Sx ^ Sy -> Dz, clear LSW of Dz; else: nop")
  (code "111110********** 10100111xxyyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
));


__sexpr (insn_blocks.push_back
(insns "DSP Fixed Decimal Point Multiplication Instructions"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "pmuls	Se,Sf,Dg"
  SH_DSP
  (abstract "MSW of Se * MSW of Sf -> Dg")
  (code "111110********** 0100eeff0000gg00")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
));


__sexpr (insn_blocks.push_back
(insns "DSP Shift Operation Instructions"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "psha		Sx,Sy,Dz"
  SH_DSP
  (abstract "If Sy >= 0: Sx << Sy -> Dz\nIf Sy < 0: Sx >> Sy -> Dz")
  (code "111110********** 10010001xxyyzzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct psha	Sx,Sy,Dz"
  SH_DSP
  (abstract "If DC = 1 & Sy >= 0: Sx << Sy -> Dz\nIf DC = 1 & Sy < 0: Sx >> Sy -> Dz\nIf DC = 0: nop")
  (code "111110********** 10010010xxyyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf psha	Sx,Sy,Dz"
  SH_DSP
  (abstract "If DC = 0 & Sy >= 0: Sx << Sy -> Dz\nIf DC = 0 & Sy < 0: Sx >> Sy -> Dz\nIf DC = 1: nop")
  (code "111110********** 10010011xxyyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "psha		#imm,Dz"
  SH_DSP
  (abstract "If imm >= 0: Dz << imm -> Dz\nIf imm < 0: Dz >> imm -> Dz")
  (code "111110********** 00000iiiiiiizzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "pshl		Sx,Sy,Dz"
  SH_DSP
  (abstract "If Sy >= 0: Sx << Sy -> Dz, clear LSW of Dz\nIf Sy < 0: Sx >> Sy -> Dz, clear LSW of Dz")
  (code "111110********** 10000001xxyyzzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct pshl	Sx,Sy,Dz"
  SH_DSP
  (abstract "If DC = 1 & Sy >= 0: Sx << Sy -> Dz, clear LSW of Dz\nIf DC = 1 & Sy < 0: Sx >> Sy -> Dz, clear LSW of Dz\nIf DC = 0: nop")
  (code "111110********** 10000010xxyyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf pshl	Sx,Sy,Dz"
  SH_DSP
  (abstract "If DC = 0 & Sy >= 0: Sx << Sy -> Dz, clear LSW of Dz\nIf DC = 0 & Sy < 0: Sx >> Sy -> Dz, clear LSW of Dz\nIf DC = 1: nop")
  (code "111110********** 10000011xxyyzzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "pshl		#imm,Dz"
  SH_DSP
  (abstract "If imm >= 0: Dz << imm -> Dz, clear LSW of Dz\nIf imm < 0: Dz >> imm, clear LSW of Dz")
  (code "111110********** 00010iiiiiiizzzz")
  (dc_bit "Update")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
));


__sexpr (insn_blocks.push_back
(insns "DSP System Control Instructions"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "plds		Dz,MACH"
  SH_DSP
  (abstract "Dz -> MACH")
  (code "111110********** 111011010000zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "plds		Dz,MACL"
  SH_DSP
  (abstract "Dz -> MACL")
  (code "111110********** 111111010000zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct plds	Dz,MACH"
  SH_DSP
  (abstract "If DC = 1: Dz -> MACH, else: nop")
  (code "111110********** 111011100000zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct plds	Dz,MACL"
  SH_DSP
  (abstract "If DC = 1: Dz -> MACL, else: nop")
  (code "111110********** 111111100000zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf plds	Dz,MACH"
  SH_DSP
  (abstract "If DC = 0: Dz -> MACH, else: nop")
  (code "111110********** 111011110000zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf plds	Dz,MACL"
  SH_DSP
  (abstract "If DC = 0: Dz -> MACL, else: nop")
  (code "111110********** 111111110000zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "psts		MACH,Dz"
  SH_DSP
  (abstract "MACH -> Dz")
  (code "111110********** 110011010000zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "psts		MACL,Dz"
  SH_DSP
  (abstract "MACL -> Dz")
  (code "111110********** 110111010000zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct psts	MACH,Dz"
  SH_DSP
  (abstract "If DC = 1: MACH -> Dz, else: nop")
  (code "111110********** 110011100000zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dct psts	MACL,Dz"
  SH_DSP
  (abstract "If DC = 1: MACL -> Dz, else: nop")
  (code "111110********** 110111100000zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf psts	MACH,Dz"
  SH_DSP
  (abstract "If DC = 0: MACH -> Dz, else: nop")
  (code "111110********** 110011110000zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
(insn "dcf psts	MACL,Dz"
  SH_DSP
  (abstract "If DC = 0: MACL -> Dz, else: nop")
  (code "111110********** 110111110000zzzz")

  (issue SH_DSP "1")
  (latency SH_DSP "1")
)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
));


} // void build_insn_blocks (void)


