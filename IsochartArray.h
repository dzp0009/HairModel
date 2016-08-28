#ifndef _ISOCHART_ARRAY_H_INCLUDED
#define _ISOCHART_ARRAY_H_INCLUDED

#include <string.h>

template<class T>
class isochart_array {
protected:
    T *data;
    int len;
public:
    isochart_array() { data=NULL; len=0; }
    isochart_array(int l) { init(l); }
    ~isochart_array() { free(); }


    inline void init(int l);
    inline void free();
    inline void resize(int l);

    inline T& ref(int i);
    inline T& operator[](int i) { return data[i]; }
    inline T& operator()(int i) { return ref(i); }

    inline const T& ref(int i) const;
    inline const T& operator[](int i) const { return data[i]; }
    inline const T& operator()(int i) const { return ref(i); }


    inline int length() const { return len; }
    inline int maxLength() const { return len; }
};

template<class T>
inline void isochart_array<T>::init(int l)
{
    data = new T[l];
    len = l;
}

template<class T>
inline void isochart_array<T>::free()
{
    if( data )
    {
	delete[] data;
	data = NULL;
    }
}

template<class T>
inline T& isochart_array<T>::ref(int i)
{
    return data[i];
}

template<class T>
inline const T& isochart_array<T>::ref(int i) const
{
    return data[i];
}

template<class T>
inline void isochart_array<T>::resize(int l)
{
    T *old = data;
    data = new T[l];
    data = (T *)memcpy(data,old,(len < l ? len : l)*sizeof(T));
    len = l;
    delete[] old;
}

// ARRAY_H_INCLUDED
#endif
