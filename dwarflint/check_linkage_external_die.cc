/* Check that every die that has a linkage_name is also external.
   Copyright (C) 2011 Red Hat, Inc.
   This file is part of Red Hat elfutils.

   Red Hat elfutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 2 of the License.

   Red Hat elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with Red Hat elfutils; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301 USA.

   Red Hat elfutils is an included package of the Open Invention Network.
   An included package of the Open Invention Network is a package for which
   Open Invention Network licensees cross-license their patents.  No patent
   license is granted, either expressly or impliedly, by designation as an
   included package.  Should you wish to participate in the Open Invention
   Network licensing program, please visit www.openinventionnetwork.com
   <http://www.openinventionnetwork.com>.  */

#include "check_die_tree.hh"
#include "pri.hh"
#include "messages.hh"


#include "../libelf/gelf.h"
#include "../libdw/libdw.h"

using elfutils::dwarf;

namespace
{
  class check_linkage_external_die
    : public die_check
  {
  private:
    std::map<std::string, bool> _m_symbols;

  public:
    static checkdescriptor const *descriptor ()
    {
      static checkdescriptor cd
	(checkdescriptor::create ("check_linkage_external_die")
	 .description ("Check that each DIE that has a linkage_name "
		       "also has an external attribute.\n"));
      return &cd;
    }

    check_linkage_external_die (highlevel_check_i *check,
				checkstack &, dwarflint &)
    {
      // Extract all symbol table names for objects and functions
      // and store whether they are global or not in _m_symbols.
      Dwarf *dwarf = check->c_dw;
      Elf *elf = dwarf_getelf (dwarf);
      Elf_Scn *scn = NULL;
      while ((scn = elf_nextscn (elf, scn)) != NULL)
	{
	  GElf_Shdr shdr_mem;
	  GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
	  if (shdr != NULL && (shdr->sh_type == SHT_DYNSYM
			       || shdr->sh_type == SHT_SYMTAB))
	    {
	      Elf_Data *data = elf_getdata (scn, NULL);
	      size_t shstrndx;
	      elf_getshdrstrndx (elf, &shstrndx);
	      unsigned int syms = shdr->sh_size / shdr->sh_entsize;
	      for (unsigned int cnt = 0; cnt < syms; ++cnt)
		{
		  GElf_Sym sym_mem;
		  GElf_Sym *sym = gelf_getsym (data, cnt, &sym_mem);
		  if (sym != NULL
		      && (GELF_ST_TYPE (sym->st_info) == STT_OBJECT
			  || GELF_ST_TYPE (sym->st_info) == STT_FUNC))
		    {
		      const char *name;
		      name = elf_strptr (elf, shdr->sh_link, sym->st_name);
		      if (name != NULL)
			{
			  // Regard anything not explicitly marked as local
			  // a global symbol, it could be STB_GLOBAL,
			  // STB_WEAK, STB_GNU_UNIQUE, ...
			  unsigned int binding = GELF_ST_BIND (sym->st_info);
			  bool global = binding != STB_LOCAL;
			  using namespace std;
			  _m_symbols.insert (pair<string, bool>
					     (string (name), global));
			}
		    }
		}
	    }
	}
    }

    static bool is_external (all_dies_iterator<dwarf> const &it)
    {
      bool candidates = true;
      dwarf::debug_info_entry entry = *it;
      do
	{
	  dwarf::debug_info_entry::attributes_type attrs = entry.attributes ();
	  if (attrs.find (DW_AT_external) != attrs.end ())
	    return true;

	  dwarf::debug_info_entry::attributes_type::const_iterator origin
	    = attrs.find (DW_AT_abstract_origin);
	  if (origin == attrs.end ())
	    origin = attrs.find (DW_AT_specification);

	  if (origin != attrs.end ())
	    entry = *(*origin).second.reference ();
	  else
	    candidates = false;
	}
      while (candidates);

      return false;
    }

    virtual void
    die (all_dies_iterator<dwarf> const &it)
    {
      dwarf::debug_info_entry const &entry = *it;
      dwarf::debug_info_entry::attributes_type attrs = entry.attributes ();
      dwarf::debug_info_entry::attributes_type::const_iterator linkage_name
	= attrs.find (DW_AT_linkage_name);
      if (linkage_name == attrs.end ())
	linkage_name = attrs.find (DW_AT_MIPS_linkage_name);
      if (linkage_name != attrs.end ())
	{
	  using namespace std;
	  const char *name = (*linkage_name).second.string ();
	  map<string, bool>::iterator s = _m_symbols.find (string (name));
	  if (s == _m_symbols.end ())
	    {
	      // No symbol in table, OK, if not a defining or const object.
	      // GNU extension, anonymous structs can have a linkage_name.
	      if (attrs.find (DW_AT_declaration) == attrs.end ()
		  && attrs.find (DW_AT_const_value) == attrs.end ()
		  && ((entry.tag () != DW_TAG_structure_type
		      && entry.tag () != DW_TAG_enumeration_type)
		      || attrs.find (DW_AT_name) != attrs.end ()))
		{
		  wr_message (to_where (entry),
			      mc_impact_3 | mc_acc_suboptimal | mc_die_other)
		    .id (descriptor ())
		    << elfutils::dwarf::tags::name (entry.tag ())
		    << " has linkage_name attribute `"
		    << name << "', which is not in string table,"
		    << " but DIE is not marked as a declaration"
		    << " or const value."
		    << std::endl;
		}
	    }
	  else if ((*s).second == false)
	    {
	      // Local symbol in table, OK if not a defining object
	      // and marked external. Which means it comes from an
	      // external symbol table.
	      if (attrs.find (DW_AT_declaration) == attrs.end ()
		  && is_external (it))
		{
		  wr_message (to_where (entry),
			      mc_impact_3 | mc_acc_suboptimal | mc_die_other)
		    .id (descriptor ())
		    << elfutils::dwarf::tags::name (entry.tag ())
		    << " has linkage_name attribute `"
		    << name << "', which is a local symbol."
		    << std::endl;
		}
	    }
	  else if (! is_external (it))
	    {
	      // Global symbol in symbol table, not marked external.
	      // Always bad.
	      wr_message (to_where (entry),
			  mc_impact_3 | mc_acc_suboptimal | mc_die_other)
		.id (descriptor ())
		<< elfutils::dwarf::tags::name (entry.tag ())
		<< " has linkage_name attribute, but no external attribute."
		<< std::endl;
	    }
	}
    }
  };

  reg_die_check<check_linkage_external_die> reg;
}
