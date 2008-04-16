// This file is part of the ustl library, an STL implementation.
//
// Copyright (C) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.
//
// sistream.h
//

#ifndef SISTREAM_H_0CCA102229A49F5D65EE852E62B27CE2
#define SISTREAM_H_0CCA102229A49F5D65EE852E62B27CE2

#include "mistream.h"
#include "ustring.h"

namespace ustl {

/// \class istringstream sistream.h ustl.h
/// \ingroup TextStreams
///
/// \brief A stream that reads textual data from a memory block.
///
class istringstream : public istream {
public:
    static const size_type	c_MaxDelimiters = 16;	///< Maximum number of word delimiters.
public:
    				istringstream (void);
				istringstream (const void* p, size_type n);
    explicit			istringstream (const cmemlink& source);
    void			iread (int8_t& v);
    void			iread (int32_t& v);
    void			iread (double& v);
    void			iread (bool& v);
    void			iread (wchar_t& v);
    void			iread (string& v);
#if HAVE_INT64_T
    void			iread (int64_t& v);
#endif
#if HAVE_LONG_LONG && (!HAVE_INT64_T || SIZE_OF_LONG_LONG > 8)
    void			iread (long long& v);
#endif
    inline string		str (void) const	{ string s; s.link (*this); return (s); }
    inline void			str (const string& s)	{ link (s); }
    int				get (void);
    inline void			get (char& c)	{ c = get(); }
    void			get (char* p, size_type n, char delim = '\n');
    void			get (string& s, char delim = '\n');
    void			getline (char* p, size_type n, char delim = '\n');
    void			getline (string& s, char delim = '\n');
    void			ignore (size_type n, char delim = '\0');
    inline char			peek (void)	{ int8_t v; iread (v); ungetc(); return (v); }
    inline void			putback (char)	{ ungetc(); }
    inline void			unget (void)	{ ungetc(); }
    void			set_delimiters (const char* delimiters);
    inline void			set_base (short base);
    inline void			set_decimal_separator (char)	{ }
    inline void			set_thousand_separator (char)	{ }
    void			read (void* buffer, size_type size);
    void			read (memlink& buf);
    inline void			read_strz (string& str);
    inline void			sync (void)	{ skip (remaining()); }
protected:
    char			skip_delimiters (void);
private:
    inline bool			is_delimiter (char c) const;
    template <typename T> void	read_number (T& v);
private:
    char			m_Delimiters [c_MaxDelimiters];
    uint8_t			m_Base;
};

/// Sets the numeric base used to read numbers.
inline void istringstream::set_base (short base)
{
    m_Base = base;
}

/// Reads a null-terminated character stream. This is not allowed in this class.
inline void istringstream::read_strz (string&)
{
    assert (!"Reading nul characters is not allowed from text streams");
}

/// Reads one type as another.
template <typename RealT, typename CastT>
inline void _cast_read (istringstream& is, RealT& v)
{
    CastT cv;
    is.iread (cv);
    v = RealT (cv);
}

//----------------------------------------------------------------------

template <typename T> struct object_text_reader {
    inline void operator()(istringstream& is, T& v) const { v.text_read (is); }
};
template <typename T> struct integral_text_object_reader {
    inline void operator()(istringstream& is, T& v) const { is.iread (v); }
};
template <typename T>
inline istringstream& operator>> (istringstream& is, T& v) {
    typedef typename tm::Select <numeric_limits<T>::is_integral,
	integral_text_object_reader<T>, object_text_reader<T> >::Result object_reader_t;
    object_reader_t()(is, v);
    return (is);
}

//----------------------------------------------------------------------

template <> struct object_text_reader<string> {
    inline void operator()(istringstream& is, string& v) const { is.iread (v); }
};
#define ISTRSTREAM_CAST_OPERATOR(RealT, CastT)		\
template <> struct integral_text_object_reader<RealT> {	\
    inline void operator() (istringstream& is, RealT& v) const	\
	{ _cast_read<RealT,CastT>(is, v); }		\
};
ISTRSTREAM_CAST_OPERATOR (uint8_t,	int8_t)
ISTRSTREAM_CAST_OPERATOR (int16_t,	int32_t)
ISTRSTREAM_CAST_OPERATOR (uint16_t,	int32_t)
ISTRSTREAM_CAST_OPERATOR (uint32_t,	int32_t)
ISTRSTREAM_CAST_OPERATOR (float,	double)
#if HAVE_THREE_CHAR_TYPES
ISTRSTREAM_CAST_OPERATOR (char,		int8_t)
#endif
#if HAVE_INT64_T
ISTRSTREAM_CAST_OPERATOR (uint64_t,	int64_t)
#endif
#if SIZE_OF_LONG == SIZE_OF_INT
ISTRSTREAM_CAST_OPERATOR (long,		int)
ISTRSTREAM_CAST_OPERATOR (unsigned long,int)
#endif
#if HAVE_LONG_LONG && (!HAVE_INT64_T || SIZE_OF_LONG_LONG > 8)
ISTRSTREAM_CAST_OPERATOR (unsigned long long, long long)
#endif
#undef ISTRSTREAM_CAST_OPERATOR

} // namespace ustl

#endif

