#ifndef __JHABS_THREADID_HPP
#define __JHABS_THREADID_HPP

#include <utility> //std::pair
#include <vector> //std::vector
#include <initializer_list> //std::initializer_list
#include <sstream> //std::stringstream

#include <boost/serialization/vector.hpp>
#include <boost/archive/text_oarchive.hpp>

template <class T, class U>
class MultiID {
public:  
    //main constructor
    MultiID( const std::initializer_list<std::pair<T,U>> );
    
    //defaults
    MultiID() = default; //Needed for serialization
    ~MultiID() = default; //will be allocated in a smart pointer

    //deleted 
    MultiID(const MultiID &) = delete;
    MultiID(MultiID &&) = delete;
    MultiID& operator=(const MultiID& other) = delete;  //Equals
    MultiID& operator=(MultiID&&) = delete; //RValue Equals

    //Functions
    std::vector<std::pair<T,U>> as_output() const;
    std::string as_string() const;
    U get_index(const std::size_t) const;
    U get_index_by_name( const std::string& ) const;
    std::size_t size() const { return vec_size; };

    //Friend Functions
    template <class V, class W>
    friend std::ostream& operator<<(std::ostream&, const MultiID<V,W>& );

    template <class V, class W>
    friend std::ostream& operator<<(std::ostream&, const std::shared_ptr<MultiID<V,W>>& );
private:
    friend boost::serialization::access;
    template <class Archive>
    void serialize(Archive &ar, const unsigned int version ){
        ar & id_list;
        ar & vec_size;
    }
    std::vector<std::pair<T,U>> id_list;
    std::size_t vec_size;
};

//Type Aliases
using MI_STR = const char *; //can also be std::string if required
using MI_VAL = unsigned long int; //Any numeric type
using MI_ELEMENT = std::pair<MI_STR,MI_VAL>;
using ThreadID = MultiID<MI_STR,MI_VAL>;


template <class T, class U>
MultiID<T,U>::MultiID(const std::initializer_list<std::pair<T,U>> l) {
    vec_size = l.size();
    id_list.reserve( vec_size );
    //std::copy doesn't work (!?)
    for (const auto &id : l) {
        id_list.push_back ( id );
    }
}

template<class T, class U>
std::ostream& operator<<(std::ostream& os, const MultiID<T,U>& tID)
{
    os << tID.as_string();
    return os;
}

//For MultiIDs that live in shared pointers, we have an << overload that
//takes a shared pointer.  It simply dereferences the shared pointer so
//that the above overload will work.
template<class T, class U>
std::ostream& operator<<(std::ostream& os, const std::shared_ptr<MultiID<T,U>>& tID)
{
    os << *tID;
    return os;
}

template <class T, class U>
std::vector<std::pair<T,U>> MultiID<T,U>::as_output () const {
    return id_list;
}

template <class T, class U>
std::string MultiID<T,U>::as_string () const {
    // Return a string representation of the tID
    std::stringstream string_builder;
    string_builder << id_list[0].second;
    for (auto i = 1u; i < vec_size; i++) {
        string_builder << "_" << id_list[i].second;
    }
    return string_builder.str();
}

template <class T, class U>
U MultiID<T,U>::get_index( const std::size_t index ) const {
    //Check if index is too large
    if (index >= vec_size)
        throw std::logic_error ("Cannot get index, as value is too large");

    return id_list[index].second;
}

template <class T, class U>
U MultiID<T,U>::get_index_by_name ( const std::string &name ) const {
    const auto search_func = [&name] (const std::pair<T,U> &entry) {
        return entry.first == name;
    };
    const auto res = std::find_if( id_list.cbegin(), id_list.cend(), search_func);
    if (res == id_list.end()) {
        throw std::logic_error ("Couldn't find the index you searched for");
    }
    return res->second;
}
#endif
