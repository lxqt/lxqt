/*
 * svec.h
 * This file is part of qps -- Qt-based visual process status monitor
 *
 * Copyright 1997-1999 Mattias Engdegård
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef SVEC_H
#define SVEC_H

#include <cstdlib>
#include <iostream>
#include <map>

extern "C" {
typedef int (*compare_func)(const void *, const void *);
}

template <typename Key, typename T> class SHash : public std::map<Key, T>
{
  public:
    T value(Key k, T r)
    {
        using namespace std;
        typename map<Key, T>::const_iterator it;
        it = map<Key, T>::find(k);
        if (it == map<Key, T>::end())
            return r;
        return (*it).second;
    }
    void insert(Key k, T r)
    {
        using namespace std;
        map<Key, T>::insert(pair<Key, T>(k, r));
    }
    bool contains(Key k)
    {
        if (value(k, NULL) == NULL)
            return false;
        else
            return true;
    }
};

//#ifndef NDEBUG
//#define CHECK_INDICES		// Check invalid vector indices (the default)
//#endif

template <class T> class Svec
{
  public:
    Svec(int max = 16);
    Svec(const Svec<T> &s);
    ~Svec();

    Svec<T> &operator=(const Svec<T> &s);
    int size() const;
    void setSize(int newsize);
    T &operator[](int i);
    T operator[](int i) const;
    void set(int i, T val);
    void sort(int (*compare)(const T *a, const T *b));
    void add(T x);
    void insert(int index, T val);
    void remove(int index);
    void Delete(int index);
    void clear();
    void purge(); // like clear() but deletes all contents

  private:
    void grow();
    void setextend(int index, T value);
    void indexerr(int index) const;

    T *vect;
    int alloced; // # of entries allocated
    int used;    // # of entries actually used (size)
};

template <class T> inline Svec<T>::Svec(int max) : alloced(max), used(0)
{
    vect = (T *)malloc(max * sizeof(T));
}

template <class T> inline Svec<T>::~Svec() { free(vect); }

template <class T> inline int Svec<T>::size() const { return used; }

template <class T> void Svec<T>::setSize(int newsize)
{
    while (newsize > alloced)
        grow();
    used = newsize;
}

template <class T> inline T &Svec<T>::operator[](int i)
{
#ifdef CHECK_INDICES
    if (i < 0 || i >= used)
        indexerr(i);
#endif
    return vect[i];
}

template <class T> inline T Svec<T>::operator[](int i) const
{
#ifdef CHECK_INDICES
    if (i < 0 || i >= used)
        indexerr(i);
#endif
    return vect[i];
}

template <class T> inline void Svec<T>::set(int i, T val)
{
    if (i < 0 || i >= used)
        setextend(i, val);
    else
        vect[i] = val;
}

template <class T> inline void Svec<T>::add(T x)
{
    if (++used > alloced)
        grow();
    vect[used - 1] = x;
}

/*
template<class T>
inline void Svec<T>::add(T x)
{
    if(++used > alloced)
    {

    }

    vect[used - 1] = x;
}
*/

template <class T> inline void Svec<T>::clear() { used = 0; }

template <class T> inline void Svec<T>::grow()
{
    //	printf("size=%d\n",sizeof(T));
    vect = (T *)realloc(vect, (alloced *= 2) * sizeof(T));
}

template <class T> void Svec<T>::setextend(int index, T value)
{
#ifdef CHECK_INDICES
    if (index < 0)
        fatal("const Svec: negative index");
#endif
    while (index >= alloced)
        grow();
    if (index >= used)
        used = index + 1;
    vect[index] = value;
}

template <class T> Svec<T>::Svec(const Svec<T> &s)
{
    int n = s.size();
    if (n < 8)
        n = 8;
    vect = (T *)malloc(n * sizeof(T));
    alloced = n;
    used = s.size();
    for (int i = 0; i < used; i++)
        vect[i] = s[i];
}

template <class T> Svec<T> &Svec<T>::operator=(const Svec<T> &s)
{
    if (this != &s)
    {
        if (alloced < s.size())
        {
            alloced = s.size();
            vect = (T *)realloc(vect, alloced * sizeof(T));
        }
        for (int i = 0; i < s.size(); i++)
        {
            vect[i] = s.vect[i];
        }
        used = s.size();
    }
    return *this;
}

/*
template<class T>
void Svec<T>::indexerr(int index) const
{
    fatal("Svec: index out of range (%d, valid is 0..%d)", index, used - 1);
}
*/

template <class T> void Svec<T>::insert(int index, T value)
{
#ifdef CHECK_INDICES
    if (index < 0 || index > used)
        fatal("Svec: index out of range");
#endif
    if ((used + 1) > alloced)
        grow();

    /*int i;
    T v;
          v=vect[i+1];
    for(i = index; j < =used ; i++)
        vect[i+1]=v;
        vect[i+1]=vect[i]; //vect[index+1]=vect[index];
        */

    // old
    for (int j = used - 1; j >= index; j--)
        vect[j + 1] = vect[j];

    vect[index] = value;

    used++;
}

// for int,float type , no delete
template <class T> void Svec<T>::remove(int index)
{
#ifdef CHECK_INDICES
    if (index < 0 || index >= used)
        fatal("Svec: index out of range");
#endif
    for (int j = index; j < used - 1; j++)
        vect[j] = vect[j + 1];
    used--;
}

// for class type
template <class T> void Svec<T>::Delete(int index)
{
#ifdef CHECK_INDICES
    if (index < 0 || index >= used)
        fatal("Svec: index out of range");
#endif
    delete vect[index];
    for (int j = index; j < used - 1; j++)
        vect[j] = vect[j + 1];
    used--;
}

// Assuming T is "pointer to U", delete all contents.
// Warning: duplicated objects will be deleted twice --- doubleplusungood
template <class T> void Svec<T>::purge()
{
    for (int i = 0; i < used; i++)
        delete vect[i];
    used = 0;
}

template <class T> void Svec<T>::sort(int (*compare)(const T *a, const T *b))
{
    qsort(vect, used, sizeof(T), (compare_func)compare);
}

#endif // SVEC_H
