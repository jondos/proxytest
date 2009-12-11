#ifndef __ARRAY__
#define __ARRAY__

typedef void (*ARRAY_ELEMENT_INIT_FNC)(void **);
typedef void (*ARRAY_ELEMENT_DELETE_FNC)(void *);

template<typename T>
class Array
{
    public:
        Array();
        Array(int a_sSize);
        ~Array();

        T* array();

        int clear();

        int size();

        T get(int a_sIdx);
        int set(int a_sIdx, T a_value);

        int setInitFnc(ARRAY_ELEMENT_INIT_FNC a_fnc);
        int setDeleteFnc(ARRAY_ELEMENT_DELETE_FNC a_fnc);

        int setSize(int a_sSize);

    private:
        int m_sSize;
        T* m_pArr;

        ARRAY_ELEMENT_INIT_FNC m_fncInit;
        ARRAY_ELEMENT_DELETE_FNC m_fncDelete;

        int init();
};

template<typename T>
Array<T>::Array()
{
    m_sSize = -1;
    m_pArr = NULL;
    m_fncInit = NULL;
    m_fncDelete = NULL;
}

template<typename T>
Array<T>::Array(int a_sSize)
{
    m_sSize = -1;
    m_pArr = NULL;
    m_fncInit = NULL;
    m_fncDelete = NULL;

    setSize(a_sSize);
}

template<typename T>
T* Array<T>::array()
{
    return m_pArr;
}

template<typename T>
Array<T>::~Array()
{
    clear();
}

template<typename T>
int Array<T>::init()
{
    if (m_fncInit == NULL)
        return 0;

    for (int i = 0; i < m_sSize; i++)
        m_fncInit((void**)&(m_pArr[i]));

    return 0;
}

template<typename T>
int Array<T>::clear()
{
    if (m_pArr == NULL)
        return 0;

    if (m_fncDelete != NULL)
        for (int i = 0; i < m_sSize; i++)
            m_fncDelete((void*)m_pArr[i]);

    delete[] m_pArr;
    m_pArr = NULL;
    m_sSize = -1;

    return 0;
}

template<typename T>
int Array<T>::setInitFnc(ARRAY_ELEMENT_INIT_FNC a_fnc)
{
    m_fncInit = a_fnc;

    return init();
}

template<typename T>
int Array<T>::setDeleteFnc(ARRAY_ELEMENT_DELETE_FNC a_fnc)
{
    m_fncDelete = a_fnc;
    return 0;
}

template<typename T>
int Array<T>::size()
{
    return m_sSize;
}

template<typename T>
T Array<T>::get(int a_sIdx)
{
    if ((a_sIdx >= 0) && (a_sIdx < m_sSize))
        return m_pArr[a_sIdx];
}

template<typename T>
int Array<T>::set(int a_sIdx, T a_value)
{
    if ((a_sIdx < 0) || (a_sIdx >= m_sSize))
        return -1;

    if (m_fncDelete != NULL)
        m_fncDelete((void*)m_pArr[a_sIdx]);

    m_pArr[a_sIdx] = a_value;

    return 0;
}

template<typename T>
int Array<T>::setSize(int a_sSize)
{
    if (a_sSize < 1)
        return -1;

    int result = 0;

    result = clear();

    if (result == 0)
        if ((m_pArr = new T[a_sSize]) == NULL)
            result = -1;

    if (result == 0)
        result = init();

    if (result == 0)
        m_sSize = a_sSize;

    return result;
}

#endif
