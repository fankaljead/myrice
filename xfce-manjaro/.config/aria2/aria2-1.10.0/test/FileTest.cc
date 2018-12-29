#include "File.h"
#include "TestUtil.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <fstream>
#include <cppunit/extensions/HelperMacros.h>

namespace aria2 {

class FileTest:public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(FileTest);
  CPPUNIT_TEST(testExists);
  CPPUNIT_TEST(testIsFile);
  CPPUNIT_TEST(testIsDir);
  CPPUNIT_TEST(testRemove);
  CPPUNIT_TEST(testSize);
  CPPUNIT_TEST(testMkdir);
  CPPUNIT_TEST(testGetDirname);
  CPPUNIT_TEST(testGetBasename);
  CPPUNIT_TEST(testRenameTo);
  CPPUNIT_TEST(testUtime);
  CPPUNIT_TEST_SUITE_END();
private:

public:
  void setUp() {
  }

  void testExists();
  void testIsFile();
  void testIsDir();
  void testRemove();
  void testSize();
  void testMkdir();
  void testGetDirname();
  void testGetBasename();
  void testRenameTo();
  void testUtime();
};


CPPUNIT_TEST_SUITE_REGISTRATION( FileTest );

void FileTest::testExists() {
  File f("FileTest.cc");
  CPPUNIT_ASSERT(f.exists());

  File f2("NonExistentFile");
  CPPUNIT_ASSERT(!f2.exists());

  File d1("../test");
  CPPUNIT_ASSERT(d1.exists());
}

void FileTest::testIsFile() {
  File f("FileTest.cc");
  CPPUNIT_ASSERT(f.isFile());

  File f2("NonExistentFile");
  CPPUNIT_ASSERT(!f2.isFile());

  File d1("../test");
  CPPUNIT_ASSERT(!d1.isFile());
}

void FileTest::testIsDir() {
  File f("FileTest.cc");
  CPPUNIT_ASSERT(!f.isDir());

  File f2("NonExistentFile");
  CPPUNIT_ASSERT(!f2.isDir());

  File d1("../test");
  CPPUNIT_ASSERT(d1.isDir());
}

void FileTest::testRemove() {
  int fd;
  std::string name = "./aria2_FileTest_testRemove_testregfile";
  unlink(name.c_str());
  if((fd = creat(name.c_str(), S_IRUSR|S_IWUSR)) < 0) {
    CPPUNIT_FAIL("cannot create test file");
  }
  close(fd);
  File f(name);
  CPPUNIT_ASSERT(f.isFile());
  CPPUNIT_ASSERT(f.remove());
  CPPUNIT_ASSERT(!f.exists());
  // delete the file again
  CPPUNIT_ASSERT(!f.remove());

  std::string dir = "./aria2_FileTest_testRemove_testdir";
#ifdef __MINGW32__
  mkdir(dir.c_str());
#else
  mkdir(dir.c_str(), 0777);
#endif // __MINGW32__
  File d(dir);
  CPPUNIT_ASSERT(d.exists());
  CPPUNIT_ASSERT(d.remove());
  CPPUNIT_ASSERT(!d.exists());
  // delete the directory again
  CPPUNIT_ASSERT(!d.remove());
}

void FileTest::testSize() {
  File f("4096chunk.txt");
  CPPUNIT_ASSERT_EQUAL(4096, (int)f.size());
}

void FileTest::testMkdir() {
  std::string dir = "./aria2_FileTest_testMkdir/test";
  File d(dir);
  if(d.exists()) {
    CPPUNIT_ASSERT(d.remove());
  }
  CPPUNIT_ASSERT(!d.exists());

  CPPUNIT_ASSERT(d.mkdirs());

  CPPUNIT_ASSERT(d.exists());
  // this test failes because d.mkdir returns false when the directory is
  // already exists.
  CPPUNIT_ASSERT(!d.mkdirs());
}

void FileTest::testGetDirname()
{
  {
    File f("/usr/lib");
    CPPUNIT_ASSERT_EQUAL(std::string("/usr"), f.getDirname());
  }
  {
    File f("/usr/");
    CPPUNIT_ASSERT_EQUAL(std::string("/usr"), f.getDirname());
  }
  {
    File f("usr");
    CPPUNIT_ASSERT_EQUAL(std::string("."), f.getDirname());
  }
  {
    File f("/");
    CPPUNIT_ASSERT_EQUAL(std::string("/"), f.getDirname());
  }
  {
    File f(".");
    CPPUNIT_ASSERT_EQUAL(std::string("."), f.getDirname());
  }
  {
    File f("..");
    CPPUNIT_ASSERT_EQUAL(std::string("."), f.getDirname());
  }
  {
    File f("");
    CPPUNIT_ASSERT_EQUAL(std::string(""), f.getDirname());
  }  
}

void FileTest::testGetBasename()
{
  {
    File f("/usr/lib");
    CPPUNIT_ASSERT_EQUAL(std::string("lib"), f.getBasename());
  }
  {
    File f("/usr/");
    CPPUNIT_ASSERT_EQUAL(std::string(""), f.getBasename());
  }
  {
    File f("usr");
    CPPUNIT_ASSERT_EQUAL(std::string("usr"), f.getBasename());
  }
  {
    File f("/");
    CPPUNIT_ASSERT_EQUAL(std::string(""), f.getBasename());
  }
  {
    File f(".");
    CPPUNIT_ASSERT_EQUAL(std::string("."), f.getBasename());
  }
  {
    File f("..");
    CPPUNIT_ASSERT_EQUAL(std::string(".."), f.getBasename());
  }
  {
    File f("");
    CPPUNIT_ASSERT_EQUAL(std::string(""), f.getBasename());
  }
}

void FileTest::testRenameTo()
{
  std::string fname = "aria2_FileTest_testRenameTo.txt";
  std::ofstream of(fname.c_str(), std::ios::binary);
  of.close();

  File f(fname);
  std::string fnameTo = "aria2_FileTest_testRenameTo_dest.txt";
  CPPUNIT_ASSERT(f.renameTo(fnameTo));
  CPPUNIT_ASSERT(f.exists());
  CPPUNIT_ASSERT(!File(fname).exists());
  CPPUNIT_ASSERT_EQUAL(fnameTo, f.getBasename());

  // to see renameTo() work even when the destination file exists
  of.open(fname.c_str());
  of.close();
  
  CPPUNIT_ASSERT(f.renameTo(fname));
}

void FileTest::testUtime()
{
  File f("./aria2_FileTest_testUTime");
  createFile(f.getPath(), 0);
  
  time_t atime = (time_t) 100000;
  time_t mtime = (time_t) 200000;
  
  CPPUNIT_ASSERT(f.utime(Time(atime), Time(mtime)));
  
  a2_struct_stat buf;
  CPPUNIT_ASSERT(0 == a2stat(f.getPath().c_str(), &buf));
  CPPUNIT_ASSERT_EQUAL((time_t)atime, buf.st_atime);
  CPPUNIT_ASSERT_EQUAL((time_t)mtime, f.getModifiedTime().getTime());

  File notFound("./aria2_FileTest_testUTime_notFound");
  notFound.remove();
  CPPUNIT_ASSERT(!notFound.utime(Time(atime), Time(mtime)));
}

} // namespace aria2
