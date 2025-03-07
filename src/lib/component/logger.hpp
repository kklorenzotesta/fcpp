// Copyright © 2021 Giorgio Audrito. All Rights Reserved.

/**
 * @file logger.hpp
 * @brief Implementation of the `logger` component logging summarisations of nodes.
 */

#ifndef FCPP_COMPONENT_LOGGER_H_
#define FCPP_COMPONENT_LOGGER_H_

#include <cassert>
#include <cstddef>
#include <ctime>

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "lib/common/plot.hpp"
#include "lib/component/base.hpp"
#include "lib/option/aggregator.hpp"
#include "lib/option/sequence.hpp"


/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {


//! @brief Namespace for all FCPP components.
namespace component {


//! @brief Namespace of tags to be used for initialising components.
namespace tags {
    //! @brief Declaration tag associating to a sequence of storage tags and corresponding aggregator types.
    template <typename... Ts>
    struct aggregators {};

    //! @brief Declaration tag associating to a sequence of initialisation tags to be fed to plotters.
    template <typename... Ts>
    struct extra_info {};

    //! @brief Declaration tag associating to a sequence generator type scheduling writing of data.
    template <typename T>
    struct log_schedule {};

    //! @brief Declaration tag associating to a plot type.
    template <typename T>
    struct plot_type {};

    //! @brief Declaration tag associating to an output stream type
    template <typename T>
    struct ostream_type {};

    //! @brief Declaration flag associating to whether parallelism is enabled.
    template <bool b>
    struct parallel;

    //! @brief Declaration flag associating to whether new values are pushed to aggregators or pulled when needed.
    template <bool b>
    struct value_push {};

    //! @brief Net initialisation tag associating to the main name of a component composition instance.
    struct name {};

    //! @brief Net initialisation tag associating to an output stream for logging.
    struct output {};

    //! @brief Net initialisation tag associating to a pointer to a plotter object.
    struct plotter {};

    //! @brief Net initialisation tag associating to the number of threads that can be created.
    struct threads;
}


//! @cond INTERNAL
namespace details {
    //! @brief Makes a stream reference from a `std::string` path.
    template <typename S, typename T>
    std::shared_ptr<std::ostream> make_stream(std::string const& s, common::tagged_tuple<S,T> const& t) {
        if (s.back() == '/' or s.back() == '\\') {
            std::stringstream ss;
            ss << s;
            std::string name{common::get_or<tags::name>(t, "")};
            if (name.size() > 0)
                ss << name << "_";
            t.print(ss, common::underscore_tuple, common::skip_tags<tags::name,tags::output,tags::plotter>);
            ss << ".txt";
            return std::shared_ptr<std::ostream>(new std::ofstream(ss.str()));
        } else return std::shared_ptr<std::ostream>(new std::ofstream(s));
    }
    //! @brief Makes a stream reference from a `const char*` path.
    template <typename S, typename T>
    std::shared_ptr<std::ostream> make_stream(const char* s, common::tagged_tuple<S,T> const& t) {
        return make_stream(std::string(s), t);
    }
    //! @brief Makes a stream reference from a stream pointer.
    template <typename O, typename S, typename T>
    std::shared_ptr<O> make_stream(O* o, const common::tagged_tuple<S,T>&) {
        return std::shared_ptr<O>(o, [] (void*) {});
    }
    //! @brief Makes a reference to a plotter.
    template <typename T>
    std::shared_ptr<T> make_plotter(T* o) {
        return std::shared_ptr<T>(o, [] (void*) {});
    }
    //! @brief Makes a reference to a plotter.
    template <typename T>
    std::shared_ptr<T> make_plotter(std::nullptr_t) {
        return std::shared_ptr<T>(new T());
    }
    //! @brief Computes the row type given a the aggregator tuple (general case).
    template <typename T>
    struct row_type;
//! @brief Computes the row type given a the aggregator tuple.
    template <typename... Ts, typename... Ss>
    struct row_type<common::tagged_tuple<common::type_sequence<Ts...>, common::type_sequence<Ss...>>> {
        using type = common::tagged_tuple_cat<typename Ss::template result_type<Ts>...>;
    };
}
//! @endcond


/**
 * @brief Component logging summarisations of nodes.
 *
 * Requires a \ref storage parent component, and also an \ref identifier parent component if \ref tags::value_push is false.
 * The \ref timer component cannot be a parent of a \ref logger to preserve log scheduling.
 * If a \ref randomizer parent component is not found, \ref crand is passed to the \ref tags::log_schedule object.
 *
 * <b>Declaration tags:</b>
 * - \ref tags::aggregators defines a sequence of storage tags and corresponding aggregator types (defaults to the empty sequence).
 * - \ref tags::extra_info defines a sequence of net initialisation tags and types to be fed to plotters (defaults to the empty sequence).
 * - \ref tags::log_schedule defines a sequence generator type scheduling writing of data (defaults to \ref sequence::never).
 * - \ref tags::plot_type defines a plot type (defaults to \ref plot::none).
 * - \ref tags::clock_type defines a clock type (defaults to `std::chrono::system_clock`)
 *
 * <b>Declaration flags:</b>
 * - \ref tags::parallel defines whether parallelism is enabled (defaults to \ref FCPP_PARALLEL).
 * - \ref tags::value_push defines whether new values are pushed to aggregators or pulled when needed (defaults to \ref FCPP_VALUE_PUSH).
 *
 * <b>Net initialisation tags:</b>
 * - \ref tags::name associates to the main name of a component composition instance (defaults to the empty string).
 * - \ref tags::output associates to an output stream for logging (defaults to `std::cout`).
 * - \ref tags::plotter associates to a pointer to a plotter object (defaults to `nullptr`).
 * - \ref tags::threads associates to the number of threads that can be created (defaults to \ref FCPP_THREADS).
 *
 * If \ref tags::value_push is true, all aggregators need to support erasing and \ref tags::threads is ignored; otherwise, it requires an \ref identifier parent component.
 *
 * Overall, \ref tags::threads is ignored whenever \ref tags::value_push is true or \ref tags::parallel is false.
 *
 * Admissible values for \ref tags::output are:
 * - a pointer to a stream (as `std::ostream*`);
 * - a file name (as `std::string` or `const char*`);
 * - a directory name ending in `/` or `\`, to which a generated file name will be appended (starting with \ref tags::name followed by a representation of the whole initialisation parameters of the net instance).
 */
template <class... Ts>
struct logger {
    //! @brief Sequence of storage tags and corresponding aggregator types.
    using aggregators_type = common::option_types<tags::aggregators, Ts...>;

    //! @brief Tagged tuple type for storing extra info.
    using extra_info_type = common::tagged_tuple_t<common::option_types<tags::extra_info, Ts...>>;

    //! @brief Type of the plotter object.
    using plot_type = common::option_type<tags::plot_type, plot::none, Ts...>;

    //! @brief Type of the output stream.
    using ostream_type = common::option_type<tags::ostream_type, std::ostream, Ts ...>;

    //! @brief Type of the clock object.
    using clock_t = common::option_type<tags::clock_type, std::chrono::system_clock, Ts ...>;

    //! @brief Sequence generator type scheduling writing of data.
    using schedule_type = common::option_type<tags::log_schedule, sequence::never, Ts...>;

    //! @brief Whether parallelism is enabled.
    constexpr static bool parallel = common::option_flag<tags::parallel, FCPP_PARALLEL, Ts...>;

    //! @brief Whether new values are pushed to aggregators or pulled when needed.
    constexpr static bool value_push = common::option_flag<tags::value_push, FCPP_VALUE_PUSH, Ts...>;

    /**
     * @brief The actual component.
     *
     * Component functionalities are added to those of the parent by inheritance at multiple levels: the whole component class inherits tag for static checks of correct composition, while `node` and `net` sub-classes inherit actual behaviour.
     * Further parametrisation with F enables <a href="https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern">CRTP</a> for static emulation of virtual calls.
     *
     * @param F The final composition of all components.
     * @param P The parent component to inherit from.
     */
    template <typename F, typename P>
    struct component : public P {
        DECLARE_COMPONENT(logger);
        REQUIRE_COMPONENT(logger,storage);
        REQUIRE_COMPONENT_IF(logger,identifier, not value_push);
        AVOID_COMPONENT(logger,timer);
        CHECK_COMPONENT(randomizer);

        //! @brief The local part of the component.
        class node : public P::node {
          public: // visible by net objects and the main program
            /**
             * @brief Main constructor.
             *
             * @param n The corresponding net object.
             * @param t A `tagged_tuple` gathering initialisation values.
             */
            template <typename S, typename T>
            node(typename F::net& n, common::tagged_tuple<S,T> const& t) : P::node(n,t) {
                if (value_push) P::node::net.aggregator_insert(P::node::storage_tuple());
            }

            //! @brief Destructor erasing values from aggregators.
            ~node() {
                if (value_push) P::node::net.aggregator_erase(P::node::storage_tuple());
            }

            //! @brief Performs computations at round start with current time `t`.
            void round_start(times_t t) {
                P::node::round_start(t);
                if (value_push) P::node::net.aggregator_erase(P::node::storage_tuple());
            }

            //! @brief Performs computations at round end with current time `t`.
            void round_end(times_t t) {
                P::node::round_end(t);
                if (value_push) P::node::net.aggregator_insert(P::node::storage_tuple());
            }
        };

        //! @brief The global part of the component.
        class net : public P::net {
          public: // visible by node objects and the main program
            //! @brief Tuple type of the contents.
            using tuple_type = common::tagged_tuple_t<aggregators_type>;

            //! @brief Type for the result of an aggregation.
            using row_type = common::tagged_tuple_cat<common::tagged_tuple_t<plot::time, times_t>, typename details::row_type<tuple_type>::type, extra_info_type>;

            //! @brief Constructor from a tagged tuple.
            template <typename S, typename T>
            net(common::tagged_tuple<S,T> const& t) : P::net(t), m_stream(details::make_stream(common::get_or<tags::output>(t, &std::cout), t)), m_plotter(details::make_plotter<plot_type>(common::get_or<tags::plotter>(t, nullptr))), m_extra_info(t), m_schedule(get_generator(has_randomizer<P>{}, *this),t), m_threads(common::get_or<tags::threads>(t, FCPP_THREADS)) {
                std::time_t time = clock_t::to_time_t(clock_t::now());
                std::string tstr = std::string(ctime(&time));
                tstr.pop_back();
                *m_stream << "##########################################################\n";
                *m_stream << "# FCPP data export started at:  " << tstr << " #\n";
                *m_stream << "##########################################################\n# ";
                t.print(*m_stream, common::assignment_tuple, common::skip_tags<tags::name,tags::output,tags::plotter>);
                *m_stream << "\n#\n";
                *m_stream << "# The columns have the following meaning:\n# time ";
                print_headers(t_tags());
                *m_stream << std::endl;
            }

            //! @brief Destructor printing an export end section.
            ~net() {
                std::time_t time = clock_t::to_time_t(clock_t::now());
                std::string tstr = std::string(ctime(&time));
                tstr.pop_back();
                *m_stream << "##########################################################\n";
                *m_stream << "# FCPP data export finished at: " << tstr << " #\n";
                *m_stream << "##########################################################" << std::endl;
                maybe_clear(has_identifier<P>{}, *this);
            }

            /**
             * @brief Returns next event to schedule for the net component.
             *
             * Should correspond to the next time also during updates.
             */
            times_t next() const {
                return std::min(m_schedule.next(), P::net::next());
            }

            //! @brief Updates the internal status of net component.
            void update() {
                if (m_schedule.next() < P::net::next()) {
                    PROFILE_COUNT("logger");
                    data_puller(common::bool_pack<not value_push>(), *this);
                    *m_stream << m_schedule.next() << " ";
                    print_output(t_tags());
                    *m_stream << std::endl;
                    data_plotter(std::is_same<plot_type, plot::none>{}, t_tags{});
                    m_schedule.step(get_generator(has_randomizer<P>{}, *this));
                    if (not value_push) m_aggregators = tuple_type{};
                } else P::net::update();
            }

            //! @brief Erases data from the aggregators.
            template <typename S, typename T>
            void aggregator_erase(common::tagged_tuple<S,T> const& t) {
                assert(value_push); // disabled for pull-based loggers
                common::lock_guard<value_push and parallel> lock(m_aggregators_mutex);
                aggregator_erase_impl(m_aggregators, t, t_tags());
            }

            //! @brief Inserts data into the aggregators.
            template <typename S, typename T>
            void aggregator_insert(common::tagged_tuple<S,T> const& t) {
                assert(value_push); // disabled for pull-based loggers
                common::lock_guard<value_push and parallel> lock(m_aggregators_mutex);
                aggregator_insert_impl(m_aggregators, t, t_tags());
            }

          private: // implementation details
            //! @brief The tagged tuple tags.
            using t_tags = typename tuple_type::tags;

            //! @brief Prints the aggregator headers.
            void print_headers(common::type_sequence<>) const {}
            template <typename U, typename... Us>
            void print_headers(common::type_sequence<U,Us...>) const {
                common::get<U>(m_aggregators).header(*m_stream, common::details::strip_namespaces(common::type_name<U>()));
                print_headers(common::type_sequence<Us...>());
            }

            //! @brief Prints the aggregator headers.
            void print_output(common::type_sequence<>) const {}
            template <typename U, typename... Us>
            void print_output(common::type_sequence<U,Us...>) const {
                common::get<U>(m_aggregators).output(*m_stream);
                print_output(common::type_sequence<Us...>());
            }

            //! @brief Erases data from the aggregators.
            template <typename S, typename T, typename... Us>
            void aggregator_erase_impl(S& a, T const& t, common::type_sequence<Us...>) {
                common::details::ignore((common::get<Us>(a).erase(common::get<Us>(t)),0)...);
            }

            //! @brief Inserts data into the aggregators.
            template <typename S, typename T, typename... Us>
            void aggregator_insert_impl(S& a,  T const& t, common::type_sequence<Us...>) {
                common::details::ignore((common::get<Us>(a).insert(common::get<Us>(t)),0)...);
            }

            //! @brief Inserts an aggregator data into the aggregators.
            template <typename S, typename T, typename... Us>
            void aggregator_add_impl(S& a,  T const& t, common::type_sequence<Us...>) {
                common::details::ignore((common::get<Us>(a) += common::get<Us>(t))...);
            }

            //! @brief Returns the `randomizer` generator if available.
            template <typename N>
            inline auto& get_generator(std::true_type, N& n) {
                return n.generator();
            }

            //! @brief Returns a `crand` generator otherwise.
            template <typename N>
            inline crand get_generator(std::false_type, N&) {
                return {};
            }

            //! @brief Deletes all nodes if parent identifier.
            template <typename N>
            inline void maybe_clear(std::true_type, N& n) {
                return n.node_clear();
            }

            //! @brief Does nothing otherwise.
            template <typename N>
            inline void maybe_clear(std::false_type, N&) {}

            //! @brief Collects data actively from nodes if `identifier` is available.
            template <typename N>
            inline void data_puller(common::bool_pack<true>, N& n) {
                if (parallel == false or m_threads == 1) {
                    for (auto it = n.node_begin(); it != n.node_end(); ++it)
                        aggregator_insert_impl(m_aggregators, it->second.storage_tuple(), t_tags());
                    return;
                }
                std::vector<tuple_type> thread_aggregators(m_threads);
                auto a = n.node_begin();
                auto b = n.node_end();
                common::parallel_for(common::tags::general_execution<parallel>(m_threads), b-a, [&thread_aggregators,&a,this] (size_t i, size_t t) {
                    aggregator_insert_impl(thread_aggregators[t], a[i].second.storage_tuple(), t_tags());
                });
                for (size_t i=0; i<m_threads; ++i)
                    aggregator_add_impl(m_aggregators, thread_aggregators[i], t_tags());
            }

            //! @brief Does nothing otherwise.
            template <typename N>
            inline void data_puller(common::bool_pack<false>, N&) {}

            //! @brief Plots data if a plotter is given.
            template <typename... Us>
            inline void data_plotter(std::false_type, common::type_sequence<Us...>) const {
                row_type r = m_extra_info;
                common::get<plot::time>(r) = m_schedule.next();
                common::details::ignore((r = common::get<Us>(m_aggregators).template result<Us>())...);
                *m_plotter << r;
            }

            //! @brief Does nothing otherwise.
            template <typename U>
            inline void data_plotter(std::true_type, U) const {}

            //! @brief The stream where data is exported.
            std::shared_ptr<ostream_type> m_stream;

            //! @brief A reference to a plotter object.
            std::shared_ptr<plot_type> m_plotter;

            //! @brief Tuple storing extra information.
            extra_info_type m_extra_info;

            //! @brief The scheduling of exporting events.
            schedule_type m_schedule;

            //! @brief The aggregator tuple.
            tuple_type m_aggregators;

            //! @brief A mutex for accessing aggregation.
            common::mutex<value_push and parallel> m_aggregators_mutex;

            //! @brief The number of threads to be used.
            const size_t m_threads;
        };
    };
};


}


}

#endif // FCPP_COMPONENT_LOGGER_H_
