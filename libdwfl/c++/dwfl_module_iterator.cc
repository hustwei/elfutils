/* -*-c++-*-
   Copyright (C) 2009, 2010, 2011, 2012, 2014, 2015 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#include "libdwfl"
#include "libdwflP.hh"

struct elfutils::dwfl_module_iterator::pimpl
{
  Dwfl *m_dwfl;
  ptrdiff_t m_offset;
  Dwfl_Module *m_module;

  static int
  module_cb (Dwfl_Module *mod, void **, const char *,
	     Dwarf_Addr, void *arg)
  {
    pimpl *self = static_cast <pimpl *> (arg);
    self->m_module = mod;
    return DWARF_CB_ABORT;
  }

  void
  move ()
  {
    m_offset = dwfl_getmodules (m_dwfl, module_cb, this, m_offset);
    if (m_offset == -1)
      throw_libdwfl ();
  }

  explicit pimpl (ptrdiff_t off)
    : m_dwfl (NULL)
    , m_offset (off)
  {}

  explicit pimpl (Dwfl *dwfl)
    : m_dwfl (dwfl)
    , m_offset (0)
  {
    move ();
  }

  pimpl (pimpl const &that)
    : m_dwfl (that.m_dwfl)
    , m_offset (that.m_offset)
    , m_module (that.m_module)
  {}

  bool
  operator== (pimpl const &that) const
  {
    assert (m_dwfl == NULL || that.m_dwfl == NULL || m_dwfl == that.m_dwfl);
    return m_offset == that.m_offset;
  }
};

elfutils::dwfl_module_iterator::dwfl_module_iterator (ptrdiff_t off)
  : m_pimpl (new pimpl (off))
{}

elfutils::dwfl_module_iterator::dwfl_module_iterator (Dwfl *dwfl)
  : m_pimpl (new pimpl (dwfl))
{}

elfutils::dwfl_module_iterator::dwfl_module_iterator (dwfl_module_iterator const &that)
  : m_pimpl (new pimpl (*that.m_pimpl))
{}

elfutils::dwfl_module_iterator::~dwfl_module_iterator ()
{
  delete m_pimpl;
}

elfutils::dwfl_module_iterator &
elfutils::dwfl_module_iterator::operator= (dwfl_module_iterator const &that)
{
  if (this != &that)
    {
      pimpl *npimpl = new pimpl (*that.m_pimpl);
      delete m_pimpl;
      m_pimpl = npimpl;
    }

  return *this;
}

elfutils::dwfl_module_iterator
elfutils::dwfl_module_iterator::end ()
{
  return dwfl_module_iterator ((ptrdiff_t) 0);
}

elfutils::dwfl_module_iterator &
elfutils::dwfl_module_iterator::operator++ ()
{
  m_pimpl->move ();
  return *this;
}

elfutils::dwfl_module_iterator
elfutils::dwfl_module_iterator::operator++ (int)
{
  dwfl_module_iterator ret = *this;
  ++*this;
  return ret;
}

Dwfl_Module &
elfutils::dwfl_module_iterator::operator* () const
{
  return *m_pimpl->m_module;
}

Dwfl_Module *
elfutils::dwfl_module_iterator::operator-> () const
{
  return &**this;
}

bool
elfutils::dwfl_module_iterator::operator== (dwfl_module_iterator const &that) const
{
  return *m_pimpl == *that.m_pimpl;
}

bool
elfutils::dwfl_module_iterator::operator!= (dwfl_module_iterator const &that) const
{
  return ! (*this == that);
}
