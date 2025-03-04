// Copyright © 2021 Giorgio Audrito. All Rights Reserved.

/**
 * @file twin.hpp
 * @brief Implementation of the `twin` class for handling two variables, possibly identical.
 */

#ifndef FCPP_INTERNAL_TWIN_H_
#define FCPP_INTERNAL_TWIN_H_

#include <type_traits>
#include <utility>


/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {


//! @brief Namespace containing objects of internal use.
namespace internal {


//! @cond INTERNAL
template <typename T, bool is_twin>
class twin;
//! @endcond


//! @brief Class offering access to a single value through `first()` and `second()`.
template <typename T>
class twin<T, true> {
  public:
    //! @brief The type of the content.
    typedef T value_type;

  private:
    //! @brief The content of the class.
    T m_data;

  public:
    //! @name constructors
    //! @{
    //! @brief Default constructor.
    twin() = default;

    //! @brief Copy constructor.
    twin(const twin<T, true>&) = default;

    //! @brief Move constructor.
    twin(twin<T, true>&&) = default;

    //! @brief Initialising constructor.
    template <typename... Ts, typename = std::enable_if_t<not std::is_same<std::tuple<std::decay_t<Ts>...>, std::tuple<twin>>::value>>
    twin(Ts&&... xs) : m_data(std::forward<Ts>(xs)...) {}
    //! @}

    //! @name assignment operators
    //! @{
    //! @brief Copy assignment.
    twin<T, true>& operator=(const twin<T, true>&) = default;

    //! @brief Move assignment.
    twin<T, true>& operator=(twin<T, true>&&) = default;
    //! @}

    //! @brief Equality operator.
    bool operator==(const twin<T, true>& o) const {
        return m_data == o.m_data;
    }

    //! @brief Access to the first element.
    T& first() {
        return m_data;
    }

    //! @brief Const access to the first element.
    T const& first() const {
        return m_data;
    }

    //! @brief Access to the second element.
    T& second() {
        return m_data;
    }

    //! @brief Const access to the second element.
    T const& second() const {
        return m_data;
    }
};


//! @brief Class offering access to different values through `first()` and `second()`.
template <typename T>
class twin<T, false> {
  public:
    //! @brief The type of the content.
    typedef T value_type;

  private:
    //! @brief The content of the class.
    T m_data1, m_data2;

  public:
    //! @name constructors
    //! @{
    //! @brief Default constructor.
    twin() = default;

    //! @brief Copy constructor.
    twin(const twin<T, false>&) = default;

    //! @brief Move constructor.
    twin(twin<T, false>&&) = default;

    //! @brief Initialising constructor.
    template <typename... Ts, typename = std::enable_if_t<not std::is_same<std::tuple<std::decay_t<Ts>...>, std::tuple<twin>>::value>>
    twin(Ts&&... xs) : m_data1(std::forward<Ts>(xs)...), m_data2(std::forward<Ts>(xs)...) {}
    //! @}

    //! @name assignment operators
    //! @{
    //! @brief Copy assignment.
    twin<T, false>& operator=(const twin<T, false>&) = default;

    //! @brief Move assignment.
    twin<T, false>& operator=(twin<T, false>&&) = default;
    //! @}

    //! @brief Equality operator.
    bool operator==(const twin<T, false>& o) const {
        return m_data1 == o.m_data1 && m_data2 == o.m_data2;
    }

    //! @brief Access to the first element.
    T& first() {
        return m_data1;
    }

    //! @brief Const access to the first element.
    T const& first() const {
        return m_data1;
    }

    //! @brief Access to the second element.
    T& second() {
        return m_data2;
    }

    //! @brief Const access to the second element.
    T const& second() const {
        return m_data2;
    }
};


}


}

#endif // FCPP_INTERNAL_TWIN_H_
