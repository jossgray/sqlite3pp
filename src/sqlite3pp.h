// sqlite3pp.h
//
// The MIT License
//
// Copyright (c) 2015 Wongoo Lee (iwongu at gmail dot com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef SQLITE3PP_H
#define SQLITE3PP_H

#include <functional>
#include <iterator>
#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <tuple>

namespace sqlite3pp
{
  namespace ext
  {
    class function;
    class aggregate;
  }

  class null_type {};
  extern null_type ignore;

  class noncopyable
  {
   protected:
    constexpr noncopyable() = default;
    ~noncopyable() = default;

    noncopyable(noncopyable const&) = delete;
    noncopyable& operator=(noncopyable const&) = delete;
  };

  class database : noncopyable
  {
    friend class statement;
    friend class database_error;
    friend class ext::function;
    friend class ext::aggregate;

   public:
    typedef std::function<int (int)> busy_handler;
    typedef std::function<int ()> commit_handler;
    typedef std::function<void ()> rollback_handler;
    typedef std::function<void (int, char const*, char const*, long long int)> update_handler;
    typedef std::function<int (int, char const*, char const*, char const*, char const*)> authorize_handler;

    explicit database(char const* dbname = nullptr);
    ~database();

    int connect(char const* dbname);
    int connect_v2(char const* dbname, int flags, char const* vfs = nullptr);
    int disconnect();

    int attach(char const* dbname, char const* name);
    int detach(char const* name);

    long long int last_insert_rowid() const;

    int error_code() const;
    char const* error_msg() const;

    int execute(char const* sql);
    int executef(char const* sql, ...);

    int set_busy_timeout(int ms);

    void set_busy_handler(busy_handler h);
    void set_commit_handler(commit_handler h);
    void set_rollback_handler(rollback_handler h);
    void set_update_handler(update_handler h);
    void set_authorize_handler(authorize_handler h);

   private:
    sqlite3* db_;

    busy_handler bh_;
    commit_handler ch_;
    rollback_handler rh_;
    update_handler uh_;
    authorize_handler ah_;
  };

  class database_error : public std::runtime_error
  {
   public:
    explicit database_error(char const* msg);
    explicit database_error(database& db);
  };

  class statement : noncopyable
  {
   public:
    int prepare(char const* stmt);
    int finish();

    int bind(int idx, int value);
    int bind(int idx, double value);
    int bind(int idx, long long int value);
    int bind(int idx, char const* value, bool fstatic = true);
    int bind(int idx, void const* value, int n, bool fstatic = true);
    int bind(int idx);
    int bind(int idx, null_type);

    int bind(char const* name, int value);
    int bind(char const* name, double value);
    int bind(char const* name, long long int value);
    int bind(char const* name, char const* value, bool fstatic = true);
    int bind(char const* name, void const* value, int n, bool fstatic = true);
    int bind(char const* name);
    int bind(char const* name, null_type);

    int step();
    int reset();

   protected:
    explicit statement(database& db, char const* stmt = nullptr);
    ~statement();

    int prepare_impl(char const* stmt);
    int finish_impl(sqlite3_stmt* stmt);

   protected:
    database& db_;
    sqlite3_stmt* stmt_;
    char const* tail_;
  };

  class command : public statement
  {
   public:
    class bindstream
    {
     public:
      bindstream(command& cmd, int idx);

      template <class T>
      bindstream& operator << (T value) {
        auto rc = cmd_.bind(idx_, value);
        if (rc != SQLITE_OK) {
          throw database_error(cmd_.db_);
        }
        ++idx_;
        return *this;
      }

     private:
      command& cmd_;
      int idx_;
    };

    explicit command(database& db, char const* stmt = nullptr);

    bindstream binder(int idx = 1);

    int execute();
    int execute_all();
  };

  class query : public statement
  {
   public:
    class rows
    {
     public:
      class getstream
      {
       public:
        getstream(rows* rws, int idx);

        template <class T>
        getstream& operator >> (T& value) {
          value = rws_->get(idx_, T());
          ++idx_;
          return *this;
        }

       private:
        rows* rws_;
        int idx_;
      };

      explicit rows(sqlite3_stmt* stmt);

      int data_count() const;
      int column_type(int idx) const;

      int column_bytes(int idx) const;

      template <class T> T get(int idx) const {
        return get(idx, T());
      }

      template <class T1>
      std::tuple<T1> get_columns(int idx1) const {
        return std::make_tuple(get(idx1, T1()));
      }

      template <class T1, class T2>
      std::tuple<T1, T2> get_columns(int idx1, int idx2) const {
        return std::make_tuple(get(idx1, T1()), get(idx2, T2()));
      }

      template <class T1, class T2, class T3>
      std::tuple<T1, T2, T3> get_columns(int idx1, int idx2, int idx3) const {
        return std::make_tuple(get(idx1, T1()), get(idx2, T2()), get(idx3, T3()));
      }

      template <class T1, class T2, class T3, class T4>
      std::tuple<T1, T2, T3, T4> get_columns(int idx1, int idx2, int idx3, int idx4) const {
        return std::make_tuple(get(idx1, T1()), get(idx2, T2()), get(idx3, T3()), get(idx4, T4()));
      }

      template <class T1, class T2, class T3, class T4, class T5>
      std::tuple<T1, T2, T3, T4, T5> get_columns(int idx1, int idx2, int idx3, int idx4, int idx5) const {
        return std::make_tuple(get(idx1, T1()), get(idx2, T2()), get(idx3, T3()), get(idx4, T4()), get(idx5, T5()));
      }

      template <class T1, class T2, class T3, class T4, class T5, class T6>
      std::tuple<T1, T2, T3, T4, T5, T6> get_columns(int idx1, int idx2, int idx3, int idx4, int idx5, int idx6) const {
        return std::make_tuple(get(idx1, T1()), get(idx2, T2()), get(idx3, T3()), get(idx4, T4()), get(idx5, T5()), get(idx6, T6()));
      }

      template <class T1, class T2, class T3, class T4, class T5, class T6, class T7>
      std::tuple<T1, T2, T3, T4, T5, T6, T7> get_columns(int idx1, int idx2, int idx3, int idx4, int idx5, int idx6, int idx7) const {
        return std::make_tuple(get(idx1, T1()), get(idx2, T2()), get(idx3, T3()), get(idx4, T4()), get(idx5, T5()), get(idx6, T6()), get(idx7, T7()));
      }

      template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
      std::tuple<T1, T2, T3, T4, T5, T6, T7, T8> get_columns(int idx1, int idx2, int idx3, int idx4, int idx5, int idx6, int idx7, int idx8) const {
        return std::make_tuple(get(idx1, T1()), get(idx2, T2()), get(idx3, T3()), get(idx4, T4()), get(idx5, T5()), get(idx6, T6()), get(idx7, T7()), get(idx8, T8()));
      }

      getstream getter(int idx = 0);

     private:
      int get(int idx, int) const;
      double get(int idx, double) const;
      long long int get(int idx, long long int) const;
      char const* get(int idx, char const*) const;
      std::string get(int idx, std::string) const;
      void const* get(int idx, void const*) const;
      null_type get(int idx, null_type) const;

     private:
      sqlite3_stmt* stmt_;
    };

    class query_iterator
      : public std::iterator<std::input_iterator_tag, rows>
    {
     public:
      query_iterator();
      explicit query_iterator(query* cmd);

      bool operator==(query_iterator const&) const;
      bool operator!=(query_iterator const&) const;

      query_iterator& operator++();

      value_type operator*() const;

     private:
      query* cmd_;
      int rc_;
    };

    explicit query(database& db, char const* stmt = nullptr);

    int column_count() const;

    char const* column_name(int idx) const;
    char const* column_decltype(int idx) const;

    typedef query_iterator iterator;

    iterator begin();
    iterator end();
  };

  class transaction : noncopyable
  {
   public:
    explicit transaction(database& db, bool fcommit = false, bool freserve = false);
    ~transaction();

    int commit();
    int rollback();

   private:
    database* db_;
    bool fcommit_;
  };

} // namespace sqlite3pp

#endif
