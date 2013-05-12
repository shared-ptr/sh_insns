
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <exception>
#include <cassert>

std::istream& skip_spaces (std::istream& in)
{
  while (in.good () && std::isspace (in.peek ()))
    in.get ();
  return in;
}

static const std::string ID_STR = "__sexpr";

struct expr
{
  std::vector<expr> args;
  std::string name;

  expr (void) = default;
  expr (const std::string& n) : name (n) { }

  bool is_symbol (void) const
  {
    return args.empty ();
  }

  void print (std::ostream& out, int indent = 0) const
  {
    if (is_symbol ())
      out << name << " ";

    else
    {
      out << " ([" << args.size () << "]  ";
      bool first = true;
      for (const auto& a : args)
      {
        a.print (out);
//        if (!first)
//	  out << " | ";
	first = false;
      }
      out << ") ";
    }
  }

  void parse (std::istream& in, size_t max_args = std::numeric_limits<size_t>::max ())
  {
//    std::cout << "\nparse " << this;
    skip_spaces (in);
    std::string cur_symbol;
    int in_string = 0;
    int in_raw_string = 0;

    while (true)
    {
      char c;
      in >> std::noskipws >> c;
      if (!in.good ())
        return;

      // skip preprocessor directives (line, file markers)
      if (c == '#' && in_string == 0 && in_raw_string == 0)
      {
        in.ignore (std::numeric_limits<std::streamsize>::max (), '\n');
        continue;
      }

      if (c == '{' /* && in_raw_string == 0 */ )
      {
        if (in_raw_string > 0)	// output nested '{'
          cur_symbol += c;
        in_raw_string ++;
        continue;
      }

      if (c == '}' && in_raw_string)
      {
        in_raw_string --;
        if (in_raw_string > 0)
          cur_symbol += c;
        continue;
      }
      
      if (c == '"' && in_string)
      {
        in_string --;
        cur_symbol += c;
        continue;
      }

      if (c == '"' && in_string == 0)
      {
        in_string ++;
        cur_symbol += c;
        continue;
      }

      if (in_string || in_raw_string)
      {
        assert (in_string >= 0 && in_raw_string >= 0);
        cur_symbol += c;
        continue;
      }

      if ((c == '(' || c == ')' || isspace (c)) && !cur_symbol.empty ())
      {
//	    std::cout << "\ncur_symbol = " << cur_symbol;
        args.emplace_back (cur_symbol);
        cur_symbol.clear ();
      }

      if (c == '(')
      {
        args.emplace_back ();
        args.back ().parse (in);
        if (args.size () >= max_args)
          return;
        skip_spaces (in);
        continue;
      }

      if (c == ')')
        return;

      if (!isspace (c))
        cur_symbol += c;
      else
        skip_spaces (in);
    }
  }

  void transform_to_cpp_var (std::ostream& out)
  {
    out << args[1].name << " (";
    args[2].transform_to_cpp (out, false);
    out << ");";
  }

  void transform_to_cpp_func (std::ostream& out)
  {
    out << args[1].name << "\n{\n";
    args[2].transform_to_cpp (out, false);
    out << ";\n}";
  }

  void transform_to_cpp_code (std::ostream& out)
  {
    args[1].transform_to_cpp (out, false);
    out << ";";
  }

  void transform_to_cpp (std::ostream& out, bool allow_special = true)
  {
    if (allow_special && !is_symbol () && args[0].name == "var")
      transform_to_cpp_var (out);

    else if (allow_special && !is_symbol () && args[0].name == "func")
      transform_to_cpp_func (out);

    else if (allow_special && !is_symbol () && args[0].name == "code")
      transform_to_cpp_code (out);

    else if (allow_special && !is_symbol () && args[0].name == ID_STR)
      args[1].transform_to_cpp (out);
    else
    {
      if (is_symbol ())
	out << name;
      else
      {
	if (args[0].is_symbol ())
	{
	  out << args[0].name << " (";
	  for (size_t i = 1; i < args.size (); ++i)
	  {
	    args[i].transform_to_cpp (out, false);
	    if (i + 1 < args.size ())
	      out << ", ";
	  }
	  out << ")";
	}
      }
    }
  }
};

int main (void)
{
  char tmpbuf[8];
  size_t tmpbuf_sz = 0;
  int match_str_pos = 0;

  while (std::cin.good ())
  {
    char c;
    std::cin.get (c);
    if (!std::cin.good ())
      break;

    tmpbuf[tmpbuf_sz ++] = c;

    if (c == ID_STR[match_str_pos])
    {
      match_str_pos ++;

      if (match_str_pos == ID_STR.size ())
      {
	expr expr_tree;
	expr_tree.parse (std::cin, 1);

	// the first arg is a dummy list because of the "__sexpr ("
	expr_tree.args[0].transform_to_cpp (std::cout);

	tmpbuf_sz = 0;
	match_str_pos = 0;
      }
    }
    else
    {
      std::cout.write (tmpbuf, tmpbuf_sz);
      match_str_pos = 0;
      tmpbuf_sz = 0;
    }
  }

//  std::cout.flush ();
//  std::cout.close ();
//  std::cout.put (0);
//  std::cout.flush ();
  std::cout << std::endl;
  return 0;
}


#endif

/*
static const std::vector<std::string> test_inputs
{

R"_(
__sexpr (var { static const arg }
	 (arg_UIWindow (objc_type "UIWindow") (to_cpp "cocoa::ui::Window::attach_obj (%s)")
		       (cpp_type "const obj_ptr<Window>&") (to_objc "%s->objc_obj<UIWindow> ()"))) 
)_"
,
R"_(
__sexpr (func { thing define_thing (void) }
  (return (thing (name "hello") (text_a "text text"))))
)_"
,
R"_(
__sexpr (code
   (thing (name "hello") (text_a "text text")))
)_"

,
R"_(
__sexpr (func { int test_int_func (void) }
   (return ((3)5)))
)_"

,
R"_(
__sexpr (func { class test (void) } (return

(classs
  (objc_name "UIScreen") (cpp_name "Screen") (filename_prefix "uiscreen")
  (inherits_from (classs (objc_name "NSObject") (cpp_name "cocoa::ns::Object")))

  nothing

  (method (name "applicationFrame") f_const ret_Rect)
))
)_"

};
*/

