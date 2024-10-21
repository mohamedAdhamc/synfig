/* === E T L =============================================================== */
/*!	\file _handle.h
**	\brief Template Object Handle Implementation
**	\internal
**
**	\legal
**	Copyright (c) 2002 Robert B. Quattlebaum Jr.
**	Copyright (c) 2007, 2008 Chris Moore
**
**	This file is part of Synfig.
**
**	Synfig is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 2 of the License, or
**	(at your option) any later version.
**
**	Synfig is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with Synfig.  If not, see <https://www.gnu.org/licenses/>.
**	\endlegal
**
**	\note
**		This is an internal header file, included by other ETL headers.
**		You should not attempt to use it directly.
*/
/* ========================================================================= */

/* === S T A R T =========================================================== */

#ifndef __ETL__HANDLE_H
#define __ETL__HANDLE_H

/* === H E A D E R S ======================================================= */

#include <cassert>
#include <typeinfo>
#include <atomic>

/* === M A C R O S ========================================================= */

/* === T Y P E D E F S ===================================================== */

/* === C L A S S E S & S T R U C T S ======================================= */

namespace etl {

// Forward Declarations
template <class T> class handle;
template <class T> class loose_handle;
template <class T> class rhandle;


// ========================================================================
/*!	\class	shared_object _handle.h	ETL/handle
**	\brief	Shared Object Base Class
**	\see handle, loose_handle
**	\writeme
*/
class shared_object
{
private:
	mutable std::atomic<int> refcount;

public:
	shared_object(const shared_object&) = delete;
	shared_object& operator= (const shared_object&) = delete;

protected:
	shared_object() noexcept : refcount(0) { }

	virtual ~shared_object() { }

public:
	virtual void ref() const noexcept
	{
		++refcount;
	}

	//! Returns \c false if object needs to be deleted
	virtual void unref() const
	{
		--refcount;
		if (refcount == 0)
			delete this;
	}

	//! Decrease reference counter without deletion of object
	//! Returns \c false if references exceed and object should be deleted
	virtual void unref_inactive() const noexcept
	{
		--refcount;
	}

	int use_count() const noexcept { return refcount; }

}; // END of class shared_object

// ========================================================================
/*!	\class	handle _handle.h	ETL/handle
**	\brief	Object Handle
**	\see shared_object, loose_handle
**	\writeme
*/
template <class T>
class handle
{
public:

	typedef T value_type;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef int count_type;
	typedef int size_type;

protected:
	value_type *obj;		//!< Pointer to object

public:

	//! Default constructor - empty handle
	handle() noexcept : obj(nullptr) { }

	//! Constructor that constructs from a pointer to new object
	handle(pointer x):obj(x)
	{
		if(obj)
			obj->ref();
	}

	//! Default copy constructor
	handle(const handle<value_type>& x) noexcept : obj(x.get())
	{
		if(obj)
			obj->ref();
	}

	//! Handle is released on deletion
	~handle() { reset(); }

	//! Assignment operator
	handle<value_type>&
	operator=(const handle<value_type>& x) noexcept
	{
		if(x.get()==obj)
			return *this;
		// add reference before detach
		pointer xobj(x.get());
		if(xobj) xobj->ref();
		reset();
		obj=xobj;
		return *this;
	}

	//! Swaps the values of two handles without reference counts
	handle<value_type>&
	swap(handle<value_type>& x) noexcept
	{
		pointer ptr=x.obj;
		x.obj=obj;
		obj=ptr;
		return *this;
	}

	//! Handle detach procedure
	/*! unref()'s the object and sets the internal object pointer to \c nullptr */
	void
	reset() noexcept
	{
		pointer xobj(obj);
		obj = nullptr;
		if(xobj)
			xobj->unref();
	}

	bool empty() const noexcept { return !obj; }

	//! Creates a new instance of a T object and puts it in the handle.
	/*! Uses the default constructor */
	void spawn() { operator=(handle(new T())); }

	//! Returns a constant handle to our object
	handle<const value_type> constant()const { assert(obj); return *this; }

	//! Returns number of instances
	count_type
	use_count() const noexcept
		{ return obj ? obj->use_count() : 0; }

	//! Returns true if there is only one instance of the object
	bool
	unique() const noexcept
		{ /* assert(obj); */ return use_count() == 1; }

	reference
	operator*() const noexcept
		{ assert(obj); return *obj; }

	pointer
	operator->() const noexcept
		{ assert(obj); return obj; }

	//! More explicit bool cast
	explicit operator bool() const noexcept
		{ return obj != nullptr; }

	operator handle<const value_type>()const
	{ return handle<const value_type>(static_cast<const_pointer>(obj)); }

	//! <tt> static_cast\<\> </tt> wrapper
	template <class U> static handle<T> cast_static		(const handle<U> &x) { return handle<T>(static_cast		<T*>(x.get())); }
	//! <tt> dynamic_cast\<\> </tt> wrapper
	template <class U> static handle<T> cast_dynamic	(const handle<U> &x) { return handle<T>(dynamic_cast	<T*>(x.get())); }
	//! <tt> const_cast\<\> </tt> wrapper
	template <class U> static handle<T> cast_const		(const handle<U> &x) { return handle<T>(const_cast		<T*>(x.get())); }
	//! <tt> reinterpret_cast\<\> </tt> wrapper
	template <class U> static handle<T> cast_reinterpret(const handle<U> &x) { return handle<T>(reinterpret_cast<T*>(x.get())); }

	template <class U> static handle<T> cast_static		(const loose_handle<U> &x);
	template <class U> static handle<T> cast_dynamic	(const loose_handle<U> &x);
	template <class U> static handle<T> cast_const		(const loose_handle<U> &x);
	template <class U> static handle<T> cast_reinterpret(const loose_handle<U> &x);

	template <class U> static handle<T> cast_static		(const rhandle<U> &x);
	template <class U> static handle<T> cast_dynamic	(const rhandle<U> &x);
	template <class U> static handle<T> cast_const		(const rhandle<U> &x);
	template <class U> static handle<T> cast_reinterpret(const rhandle<U> &x);

	template <class U> static handle<T> cast_static		(U* x);
	template <class U> static handle<T> cast_dynamic	(U* x);
	template <class U> static handle<T> cast_const		(U* x);
	template <class U> static handle<T> cast_reinterpret(U* x);

	//! Returns pointer to the object that is being wrapped
	pointer get() const noexcept { return obj; }

	//! static_cast<> overload -- Useful for implicit casts
	template <class U>
	operator handle<U>()const
	{ return handle<U>(obj); }

	template<typename U>
	bool type_is() const
	{ return dynamic_cast<const U*>(obj); }

	template<typename U>
	U* type_pointer() const
	{ return dynamic_cast<U*>(obj); }

	template<typename U>
	bool type_equal() const
	{ return typeid(*obj) == typeid(U); }
}; // END of template class handle

// ========================================================================
/*!	\class	rshared_object _handle.h	ETL/handle
**	\brief	Replaceable Shared Object Base Class
**	\see rhandle
**	\writeme
*/
class rshared_object : public shared_object
{
	template<class T> friend class rhandle;

	mutable std::atomic<int> rrefcount;

	void *front_;
	void *back_;

protected:
	rshared_object() noexcept
		: rrefcount(0), front_(nullptr), back_(nullptr)
	{ }

public:
	rshared_object(const rshared_object &other) = delete;
	rshared_object& operator= (const rshared_object&) = delete;

	virtual void rref()const
		{ rrefcount++; }

	virtual void runref()const
	{
		assert(rrefcount>0);
		rrefcount--;
	}

	int rcount()const
		{ return rrefcount; }
}; // END of class rshared_object

// ========================================================================
/*!	\class	rhandle _handle.h	ETL/handle
**	\brief	Replaceable Object Handle
**	\see rshared_object, handle, loose_handle
**	\writeme
*/
template <class T>
class rhandle : public handle<T>
{
public:

	typedef T value_type;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef int count_type;
	typedef int size_type;

	using handle<value_type>::use_count;
	using handle<value_type>::unique;
	using handle<value_type>::operator bool;
	using handle<value_type>::get;
	using handle<value_type>::operator*;
	using handle<value_type>::operator->;

private:
	using handle<value_type>::obj;

	rhandle<value_type> *prev_;
	rhandle<value_type> *next_;

	void add_to_rlist()
	{
		assert(obj);
		obj->rref();

		// If this is the first reversible handle
		if(!obj->front_)
		{
			obj->front_=obj->back_=this;
			prev_ = next_ = nullptr;
			return;
		}

		prev_=reinterpret_cast<rhandle<value_type>*>(obj->back_);
		next_ = nullptr;
		prev_->next_=this;
		obj->back_=this;
	}

	void del_from_rlist()
	{
		assert(obj);
		obj->runref();

		// If this is the last reversible handle
		if(obj->front_==obj->back_)
		{
			obj->front_ = obj->back_ = nullptr;
			prev_ = next_ = nullptr;
			return;
		}

		if(!prev_)
			obj->front_=(void*)next_;
		else
			prev_->next_=next_;

		if(!next_)
			obj->back_=(void*)prev_;
		else
			next_->prev_=prev_;
	}

public:

	//! Default constructor - empty handle
	rhandle() noexcept {}

	//! Constructor that constructs from a pointer to new object
	rhandle(pointer x):handle<T>(x)
	{
		if(obj)add_to_rlist();
	}

	rhandle(const handle<value_type> &x):handle<T>(x)
	{
		if(obj)add_to_rlist();
	}

	//! Default copy constructor
	rhandle(const rhandle<value_type> &x):handle<T>(x)
	{
		if(obj)add_to_rlist();
	}

	//! Handle is released on deletion
	~rhandle() { reset(); }

	//! Assignment operator
	rhandle<value_type> &
	operator=(const rhandle<value_type> &x)
	{
		if(x.get()==obj)
			return *this;

		reset();

		obj=x.get();
		if(obj)
		{
			obj->ref();
			add_to_rlist();
		}
		return *this;
	}

	rhandle<value_type>&
	operator=(const handle<value_type> &x)
	{
		if(x.get()==obj)
			return *this;

		reset();

		obj=x.get();
		if(obj)
		{
			obj->ref();
			add_to_rlist();
		}
		return *this;
	}

	rhandle<value_type>&
	operator=(value_type* x)
	{
		if(x==obj)
			return *this;

		reset();

		obj=x;
		if(obj)
		{
			obj->ref();
			add_to_rlist();
		}
		return *this;
	}

	//! Handle release procedure
	/*! unref()'s the object and sets the internal object pointer to \c nullptr */
	void
	reset() noexcept
	{
		if(obj)del_from_rlist();
		handle<value_type>::reset();
		obj = nullptr;
	}

	//! Creates a new instance of a T object and puts it in the handle.
	/*! Uses the default constructor */
	void spawn() { operator=(handle<value_type>(new T())); }

	//! Returns number of reversible instances
	count_type
	rcount() const noexcept
	{
		return obj?obj->rcount():0;
	}

	//! Returns true if there is only one instance of the object
	bool
	runique() const noexcept
	{
		assert(obj); return obj->front_==obj->back_;
	}

	//! \writeme
	int replace(const handle<value_type> &x)
	{
		assert(obj);
		assert(x.get()!=obj);

		if(x.get()==obj)
			return 0;

		rhandle<value_type> *iter;
		rhandle<value_type> *next;

		iter=reinterpret_cast<rhandle<value_type>*>(obj->front_);

		assert(iter);

		next=iter->next_;

		int i=0;
		#ifndef NDEBUG
		pointer obj_=obj;
		#endif

		for(; iter; iter = next, next = iter ? iter->next_ : nullptr, i++)
		{
			assert(iter->get()==obj_);
			(*iter)=x;
		}

		assert(obj==x.get());

		return i;
	}

	//! Swaps the values of two handles without reference counts
	/*!	\warning not yet implemented. \writeme */
	handle<value_type> &
	swap(handle<value_type> &x);
}; // END of template class rhandle


// ========================================================================
/*!	\class	loose_handle _handle.h	ETL/handle
**	\brief	Loose Object Handle
**	\see shared_object, handle
**	\writeme
*/
template <class T>
class loose_handle
{
public:

	typedef T value_type;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef int count_type;
	typedef int size_type;

protected:
	value_type *obj;		//!< Pointer to object

public:

	//! Default constructor - empty handle
	loose_handle() noexcept : obj(nullptr) { }

	//! Constructor that constructs from a pointer to new object
	loose_handle(pointer x) noexcept : obj(x) { }

	//! Default copy constructor
	loose_handle(const loose_handle<value_type>& x) noexcept : obj(x.get()) { }

	loose_handle(const handle<value_type> &x) noexcept : obj(x.get()) { }

	template <class U> const loose_handle<value_type>&
	operator=(const handle<U>& x) noexcept
	{
		if(x.get()==obj)
			return *this;

		obj=x.get();
		return *this;
	}

	template <class U> const loose_handle<value_type>&
	operator=(const loose_handle<U>& x) noexcept
	{
		if(x.get()==obj)
			return *this;

		obj=x.get();
		return *this;
	}

	//! Assignment operator
	const loose_handle<value_type>&
	operator=(const loose_handle<value_type>& x) noexcept
	{
		if(x.get()==obj)
			return *this;

		obj=x.get();
		return *this;
	}

	//! Swaps the values of two handles without reference counts
	loose_handle<value_type>&
	swap(loose_handle<value_type>& x) noexcept
	{
		pointer ptr=x.obj;
		x.obj=obj;
		obj=ptr;
		return *this;
	}

	//! Handle release procedure
	void reset() noexcept
	{
		obj = nullptr;
	}

	bool empty() const noexcept { return !obj; }

	//! Returns a constant handle to our object
	loose_handle<const value_type> constant()const { return *this; }

	//! Returns number of instances
	count_type
	use_count() const noexcept
		{ return obj ? obj->use_count() : 0; }

	reference
	operator*() const noexcept
		{ assert(obj); return *obj; }

	pointer
	operator->() const noexcept
		{ assert(obj); return obj; }

	//! static_cast<> overload (for consts)
	operator loose_handle<const value_type>()const
	{ return loose_handle<const value_type>(static_cast<const_pointer>(obj)); }

	operator handle<value_type>()const
	{ return handle<value_type>(obj); }

	operator rhandle<value_type>()const
	{ return rhandle<value_type>(obj); }

	//! Returns pointer to the object that is being wrapped
	pointer get() const noexcept { return obj; }

	//! More explicit bool cast
	explicit operator bool() const noexcept
		{ return obj != nullptr; }

	template<typename U>
	bool type_is() const
	{ return dynamic_cast<const U*>(obj); }

	template<typename U>
	U* type_pointer() const
	{ return dynamic_cast<U*>(obj); }

	template<typename U>
	bool type_equal() const
	{ return typeid(*obj) == typeid(U); }
}; // END of template class loose_handle

// cast loose_handle<> -> handle<>
template <class T> template <class U> handle<T> handle<T>::cast_static	   (const loose_handle<U>& x) { return handle<T>(static_cast	 <T*>(x.get())); }
template <class T> template <class U> handle<T> handle<T>::cast_dynamic	   (const loose_handle<U>& x) { return handle<T>(dynamic_cast	 <T*>(x.get())); }
template <class T> template <class U> handle<T> handle<T>::cast_const	   (const loose_handle<U>& x) { return handle<T>(const_cast		 <T*>(x.get())); }
template <class T> template <class U> handle<T> handle<T>::cast_reinterpret(const loose_handle<U>& x) { return handle<T>(reinterpret_cast<T*>(x.get())); }

// cast rhandle_handle<> -> handle<>
template <class T> template <class U> handle<T> handle<T>::cast_static	   (const rhandle<U>&	   x) { return handle<T>(static_cast	 <T*>(x.get())); }
template <class T> template <class U> handle<T> handle<T>::cast_dynamic	   (const rhandle<U>&	   x) { return handle<T>(dynamic_cast	 <T*>(x.get())); }
template <class T> template <class U> handle<T> handle<T>::cast_const	   (const rhandle<U>&	   x) { return handle<T>(const_cast		 <T*>(x.get())); }
template <class T> template <class U> handle<T> handle<T>::cast_reinterpret(const rhandle<U>&	   x) { return handle<T>(reinterpret_cast<T*>(x.get())); }

// cast U* -> handle<>
template <class T> template <class U> handle<T> handle<T>::cast_static	   (U*					   x) { return handle<T>(static_cast	 <T*>(x));		 }
template <class T> template <class U> handle<T> handle<T>::cast_dynamic	   (U*					   x) { return handle<T>(dynamic_cast	 <T*>(x));		 }
template <class T> template <class U> handle<T> handle<T>::cast_const	   (U*					   x) { return handle<T>(const_cast		 <T*>(x));		 }
template <class T> template <class U> handle<T> handle<T>::cast_reinterpret(U*					   x) { return handle<T>(reinterpret_cast<T*>(x));		 }

// operator== for handle<>, loose_handle<> and T*
template <class T,class U> bool operator==(const handle		 <T>& lhs,const handle		<U>& rhs) { return (lhs.get()==rhs.get()); }
template <class T,class U> bool operator==(const loose_handle<T>& lhs,const loose_handle<U>& rhs) { return (lhs.get()==rhs.get()); }
template <class T,class U> bool operator==(const handle		 <T>& lhs,const loose_handle<U>& rhs) { return (lhs.get()==rhs.get()); }
template <class T,class U> bool operator==(const loose_handle<T>& lhs,const handle		<U>& rhs) { return (lhs.get()==rhs.get()); }
template <class T>		   bool operator==(const handle<T>&		  lhs,const T*				 rhs) { return (lhs.get()==rhs);	   }
template <class T>		   bool operator==(const loose_handle<T>& lhs,const T*				 rhs) { return (lhs.get()==rhs);	   }
template <class T>		   bool operator==(const T*				  lhs,const handle<T>&		 rhs) { return (lhs		 ==rhs.get()); }
template <class T>		   bool operator==(const T*				  lhs,const loose_handle<T>& rhs) { return (lhs		 ==rhs.get()); }

// operator!= for handle<>, loose_handle<> and T*
template <class T,class U> bool operator!=(const handle		 <T>& lhs,const handle		<U>& rhs) { return (lhs.get()!=rhs.get()); }
template <class T,class U> bool operator!=(const loose_handle<T>& lhs,const loose_handle<U>& rhs) { return (lhs.get()!=rhs.get()); }
template <class T,class U> bool operator!=(const handle		 <T>& lhs,const loose_handle<U>& rhs) { return (lhs.get()!=rhs.get()); }
template <class T,class U> bool operator!=(const loose_handle<T>& lhs,const handle		<U>& rhs) { return (lhs.get()!=rhs.get()); }
template <class T>		   bool operator!=(const handle<T>&		  lhs,const T*				 rhs) { return (lhs.get()!=rhs);	   }
template <class T>		   bool operator!=(const loose_handle<T>& lhs,const T*				 rhs) { return (lhs.get()!=rhs);	   }
template <class T>		   bool operator!=(const T*				  lhs,const handle<T>&		 rhs) { return (lhs		 !=rhs.get()); }
template <class T>		   bool operator!=(const T*				  lhs,const loose_handle<T>& rhs) { return (lhs		 !=rhs.get()); }

// operator< for handle<>, loose_handle<> and T*
template <class T,class U> bool operator<(const handle<T>&		  lhs,const handle<U>&		 rhs) { return (lhs.get()<rhs.get());  }
template <class T,class U> bool operator<(const loose_handle<T>&  lhs,const loose_handle<U>& rhs) { return (lhs.get()<rhs.get());  }
template <class T,class U> bool operator<(const handle<T>&		  lhs,const loose_handle<U>& rhs) { return (lhs.get()<rhs.get());  }
template <class T,class U> bool operator<(const loose_handle<T>&  lhs,const handle<U>&		 rhs) { return (lhs.get()<rhs.get());  }
template <class T>		   bool operator<(const handle<T>&		  lhs,const T*				 rhs) { return (lhs.get()<rhs);		   }
template <class T>		   bool operator<(const loose_handle<T>&  lhs,const T*				 rhs) { return (lhs.get()<rhs);		   }
template <class T>		   bool operator<(const T*				  lhs,const handle<T>&		 rhs) { return (lhs		 <rhs.get());  }
template <class T>		   bool operator<(const T*				  lhs,const loose_handle<T>& rhs) { return (lhs		 <rhs.get());  }

};

/* === E N D =============================================================== */

#endif
