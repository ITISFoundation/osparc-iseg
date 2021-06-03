/// \file FastMarching.h
///
/// \author Bryn Lloyd
/// \copyright 2016/2019, IT'IS Foundation

#pragma once

#include <array>
#include <cmath>
#include <type_traits>

namespace XCore {

/**	Uniform n-dim grid
	*/
template<typename TIndex, int D>
class CUniformGridGraph
{
	template<typename T>
	static constexpr T Pow(T num, unsigned pow) { return pow == 0 ? 1 : num * Pow(num, pow - 1); }

	template<int N>
	struct Type
	{
	};

	template<typename T, int SIZE>
	class StackVector
	{
	public:
		using value_type = T;

		const T* begin() const { return &_data[0]; }
		const T* end() const { return begin() + _size; }

		T* begin() { return &_data[0]; }
		T* end() { return begin() + _size; }

		inline T& operator[](size_t idx) { return _data[idx]; }
		inline const T& operator[](size_t idx) const { return _data[idx]; }

		size_t size() const { return _size; }

		void push_back(T v)
		{
			_data[_size] = v;
			_size++;
		}

	private:
		unsigned _size = 0;
		std::array<T, SIZE> _data;
	};

public:
	using idx_type = TIndex;
	using idx_value = typename std::decay<decltype(std::declval<idx_type&>()[0])>::type;
	using idx_list_type = StackVector<idx_type, Pow(3, D) - 1>;

	CUniformGridGraph(const std::array<idx_value, D>& dims, const std::array<double, D>& spacing)
			: m_Dims(dims)
	{
		static_assert(std::is_signed<idx_value>::value, "idx_value should be signed");
		static_assert(D > 0, "Dimension should be positive");

		// total number of nodes in grid
		m_Size = 1;
		for (int k = 0; k < D; ++k)
		{
			m_Strides[k] = m_Size;
			m_Size *= dims[k];
		}
		// squared spacing
		std::transform(spacing.begin(), spacing.end(), m_Spacing2.begin(), [](double s) { return s * s; });
	}

	size_t Size() const
	{
		return m_Size;
	}

	const std::array<idx_value, D>& Shape() const
	{
		return m_Dims;
	}

	const std::array<size_t, D>& Strides() const
	{
		return m_Strides;
	}

	double Length(const idx_type& a, const idx_type& b) const
	{
		return std::sqrt(LengthD<D - 1>(Type<D - 1>(), a, b));
	}

	idx_list_type Neighbors(const idx_type& idx) const
	{
		idx_list_type neighbors;
		NeighborsD<D - 1>(idx, false, neighbors);
		return neighbors;
	}

private:
	template<int N>
	inline double LengthD(Type<N>, const idx_type& a, const idx_type& b) const
	{
		return (a[N] - b[N]) * (a[N] - b[N]) * m_Spacing2[N] + LengthD<N - 1>(a, b);
	}

	inline double LengthD(Type<-1>, const idx_type& a, const idx_type& b) const
	{
		return 0;
	}

	template<int N>
	inline void NeighborsD(const idx_type& idx, bool collect, idx_list_type& collected) const
	{
		NeighborsD(Type<N>(), idx, collect, collected);
	}

	template<int N>
	inline void NeighborsD(Type<N>, const idx_type& idx, bool collect, idx_list_type& collected) const
	{
		// make modifiable copy
		auto ref{idx};

		// center
		NeighborsD<N - 1>(ref, collect, collected);
		// bottom
		if (idx[N] > 0)
		{
			ref[N] = idx[N] - 1;
			NeighborsD<N - 1>(ref, true, collected);
		}
		// top
		if (idx[N] + 1 < m_Dims[N])
		{
			ref[N] = idx[N] + 1;
			NeighborsD<N - 1>(ref, true, collected);
		}
	}

	void NeighborsD(Type<-1>, const idx_type& idx, bool collect, idx_list_type& collected) const
	{
		if (collect)
			collected.push_back(idx);
	}

	std::array<idx_value, D> m_Dims;
	std::array<double, D> m_Spacing2;
	size_t m_Size;
	std::array<size_t, D> m_Strides;
};

} // namespace XCore
