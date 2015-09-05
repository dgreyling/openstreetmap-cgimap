#include <iostream>
#include <stdexcept>
#include <boost/noncopyable.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/optional/optional_io.hpp>

#include <sys/time.h>
#include <stdio.h>

#include "cgimap/config.hpp"
#include "cgimap/time.hpp"
#include "test_formatter.hpp"
#include "test_database.hpp"

namespace {

template <typename T>
void assert_equal(const T& a, const T&b, const std::string &message) {
  if (a != b) {
    throw std::runtime_error(
      (boost::format(
        "Expecting %1% to be equal, but %2% != %3%")
       % message % a % b).str());
  }
}

void test_single_nodes(boost::shared_ptr<data_selection> sel) {
  if (sel->check_node_visibility(1) != data_selection::exists) {
    throw std::runtime_error("Node 1 should be visible, but isn't");
  }
  if (sel->check_node_visibility(2) != data_selection::exists) {
    throw std::runtime_error("Node 2 should be visible, but isn't");
  }

  std::vector<osm_id_t> ids;
  ids.push_back(1);
  ids.push_back(2);
  ids.push_back(3);
  ids.push_back(4);
  if (sel->select_nodes(ids) != 4) {
    throw std::runtime_error("Selecting 4 nodes failed");
  }
  if (sel->select_nodes(ids) != 0) {
    throw std::runtime_error("Re-selecting 4 nodes failed");
  }

  assert_equal<data_selection::visibility_t>(
    sel->check_node_visibility(1), data_selection::exists,
    "node 1 visibility");
  assert_equal<data_selection::visibility_t>(
    sel->check_node_visibility(2), data_selection::exists,
    "node 2 visibility");
  assert_equal<data_selection::visibility_t>(
    sel->check_node_visibility(3), data_selection::deleted,
    "node 3 visibility");
  assert_equal<data_selection::visibility_t>(
    sel->check_node_visibility(4), data_selection::exists,
    "node 4 visibility");
  assert_equal<data_selection::visibility_t>(
    sel->check_node_visibility(5), data_selection::non_exist,
    "node 5 visibility");

  test_formatter f;
  sel->write_nodes(f);
  assert_equal<size_t>(f.m_nodes.size(), 4, "number of nodes written");
  assert_equal<test_formatter::node_t>(
    test_formatter::node_t(
      element_info(1, 1, 1, "2013-11-14T02:10:00Z", 1, std::string("user_1"), true),
      0.0, 0.0,
      tags_t()
      ),
    f.m_nodes[0], "first node written");
  assert_equal<test_formatter::node_t>(
    test_formatter::node_t(
      element_info(2, 1, 1, "2013-11-14T02:10:01Z", 1, std::string("user_1"), true),
      0.1, 0.1,
      tags_t()
      ),
    f.m_nodes[1], "second node written");
  assert_equal<test_formatter::node_t>(
    test_formatter::node_t(
      element_info(3, 2, 2, "2015-03-02T18:27:00Z", 1, std::string("user_1"), false),
      0.0, 0.0,
      tags_t()
      ),
    f.m_nodes[2], "third node written");
  assert_equal<test_formatter::node_t>(
    test_formatter::node_t(
      element_info(4, 1, 4, "2015-03-02T19:25:00Z", boost::none, boost::none, true),
      0.0, 0.0,
      tags_t()
      ),
    f.m_nodes[3], "fourth (anonymous) node written");
}

void test_dup_nodes(boost::shared_ptr<data_selection> sel) {
  if (sel->check_node_visibility(1) != data_selection::exists) {
    throw std::runtime_error("Node 1 should be visible, but isn't");
  }

  std::vector<osm_id_t> ids;
  ids.push_back(1);
  ids.push_back(1);
  ids.push_back(1);
  if (sel->select_nodes(ids) != 1) {
    throw std::runtime_error("Selecting 3 duplicates of 1 node failed");
  }
  if (sel->select_nodes(ids) != 0) {
    throw std::runtime_error("Re-selecting the same node failed");
  }

  assert_equal<data_selection::visibility_t>(
    sel->check_node_visibility(1), data_selection::exists,
    "node 1 visibility");

  test_formatter f;
  sel->write_nodes(f);
  assert_equal<size_t>(f.m_nodes.size(), 1, "number of nodes written");
  assert_equal<test_formatter::node_t>(
    test_formatter::node_t(
      element_info(1, 1, 1, "2013-11-14T02:10:00Z", 1, std::string("user_1"), true),
      0.0, 0.0,
      tags_t()
      ),
    f.m_nodes[0], "first node written");
}

#ifdef ENABLE_EXPERIMENTAL
void test_changeset(boost::shared_ptr<data_selection> sel) {
  assert_equal<bool>(sel->supports_changesets(), true,
                     "apidb should support changesets.");

  std::vector<osm_id_t> ids;
  ids.push_back(1);
  int num = sel->select_changesets(ids);
  assert_equal<int>(num, 1, "should have selected one changeset.");

  boost::posix_time::ptime t = parse_time("2015-09-05T17:15:33Z");

  test_formatter f;
  sel->write_changesets(f, t);
  assert_equal<size_t>(f.m_changesets.size(), 1,
                       "should have written one changeset.");

  assert_equal<test_formatter::changeset_t>(
    f.m_changesets.front(),
    test_formatter::changeset_t(
      changeset_info(
        1, // ID
        "2013-11-14T02:10:00Z", // created_at
        "2013-11-14T03:10:00Z", // closed_at
        1, // uid
        std::string("user_1"), // display_name
        boost::none, // bounding box
        2, // num_changes
        0 // comments_count
        ),
      tags_t(),
      false,
      comments_t(),
      t),
    "changesets should be equal.");
}

void test_nonpublic_changeset(boost::shared_ptr<data_selection> sel) {
  assert_equal<bool>(sel->supports_changesets(), true,
                     "apidb should support changesets.");

  std::vector<osm_id_t> ids;
  ids.push_back(4);
  int num = sel->select_changesets(ids);
  assert_equal<int>(num, 1, "should have selected one changeset.");

  boost::posix_time::ptime t = parse_time("2015-09-05T20:13:23Z");

  test_formatter f;
  sel->write_changesets(f, t);
  assert_equal<size_t>(f.m_changesets.size(), 1,
                       "should have written one changeset.");

  assert_equal<test_formatter::changeset_t>(
    f.m_changesets.front(),
    test_formatter::changeset_t(
      changeset_info(
        4, // ID
        "2013-11-14T02:10:00Z", // created_at
        "2013-11-14T03:10:00Z", // closed_at
        boost::none, // uid
        boost::none, // display_name
        boost::none, // bounding box
        2, // num_changes
        0 // comments_count
        ),
      tags_t(),
      false,
      comments_t(),
      t),
    "changesets should be equal.");
}
#endif /* ENABLE_EXPERIMENTAL */

} // anonymous namespace

int main(int, char **) {
  try {
    test_database tdb;
    tdb.setup();

    tdb.run(boost::function<void(boost::shared_ptr<data_selection>)>(
        &test_single_nodes));

    tdb.run(boost::function<void(boost::shared_ptr<data_selection>)>(
        &test_dup_nodes));

#ifndef ENABLE_EXPERIMENTAL
    tdb.run(boost::function<void(boost::shared_ptr<data_selection>)>(
              &test_changeset));

    tdb.run(boost::function<void(boost::shared_ptr<data_selection>)>(
              &test_nonpublic_changeset));
#endif /* ENABLE_EXPERIMENTAL */

  } catch (const test_database::setup_error &e) {
    std::cout << "Unable to set up test database: " << e.what() << std::endl;
    return 77;

  } catch (const std::exception &e) {
    std::cout << "Error: " << e.what() << std::endl;
    return 1;

  } catch (...) {
    std::cout << "Unknown exception type." << std::endl;
    return 99;
  }

  return 0;
}
