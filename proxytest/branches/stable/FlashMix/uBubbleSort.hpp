#ifndef uBubbleSortH
#define uBubbleSortH

template<typename BASE_TYPE>
void BubbleSort(BASE_TYPE *_Array, unsigned int _ArrayLength);

#endif

template<typename BASE_TYPE>
void BubbleSort(BASE_TYPE *_Array, unsigned int _ArrayLength) {
  for (unsigned int i = _ArrayLength - 1; i > 0; i--)
    for (unsigned int j = 0; j < i; j++)
      if (_Array[j] > _Array[j + 1]) {
        BASE_TYPE tmp = _Array[j];
        _Array[j] = _Array[j + 1];
        _Array[j + 1] = tmp;
      }
}

