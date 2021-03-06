#include "Metalink2RequestGroup.h"

#include <algorithm>

#include <cppunit/extensions/HelperMacros.h>

#include "DownloadContext.h"
#include "prefs.h"
#include "Option.h"
#include "RequestGroup.h"
#include "FileEntry.h"
#ifdef ENABLE_MESSAGE_DIGEST
# include "messageDigest.h"
#endif // ENABLE_MESSAGE_DIGEST

namespace aria2 {

class Metalink2RequestGroupTest:public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(Metalink2RequestGroupTest);
  CPPUNIT_TEST(testGenerate);
  CPPUNIT_TEST(testGenerate_groupByMetaurl);
  CPPUNIT_TEST(testGenerate_dosDirTraversal);
  CPPUNIT_TEST_SUITE_END();
private:
  SharedHandle<Option> option_;

public:
  void setUp()
  {
    option_.reset(new Option());
    option_->put(PREF_SPLIT, "1");
  }

  void testGenerate();
  void testGenerate_groupByMetaurl();
  void testGenerate_dosDirTraversal();
};


CPPUNIT_TEST_SUITE_REGISTRATION( Metalink2RequestGroupTest );

void Metalink2RequestGroupTest::testGenerate()
{
  std::vector<SharedHandle<RequestGroup> > groups;
  option_->put(PREF_DIR, "/tmp");
  Metalink2RequestGroup().generate(groups, "test.xml", option_);
  // first file
  {
    SharedHandle<RequestGroup> rg = groups[0];
    std::vector<std::string> uris;
    rg->getDownloadContext()->getFirstFileEntry()->getUris(uris);
    std::sort(uris.begin(), uris.end());
    CPPUNIT_ASSERT_EQUAL((size_t)2, uris.size());
    CPPUNIT_ASSERT_EQUAL
      (std::string("ftp://ftphost/aria2-0.5.2.tar.bz2"), uris[0]);
    CPPUNIT_ASSERT_EQUAL
      (std::string("http://httphost/aria2-0.5.2.tar.bz2"), uris[1]);

    const SharedHandle<DownloadContext>& dctx = rg->getDownloadContext();

    CPPUNIT_ASSERT(!dctx.isNull());
    CPPUNIT_ASSERT_EQUAL((uint64_t)0ULL, dctx->getTotalLength());
    CPPUNIT_ASSERT_EQUAL(std::string("/tmp"), dctx->getDir());
#ifdef ENABLE_MESSAGE_DIGEST
    CPPUNIT_ASSERT_EQUAL(MessageDigestContext::SHA1,
                         dctx->getChecksumHashAlgo());
    CPPUNIT_ASSERT_EQUAL
      (std::string("a96cf3f0266b91d87d5124cf94326422800b627d"),
       dctx->getChecksum());
#endif // ENABLE_MESSAGE_DIGEST
    CPPUNIT_ASSERT(!dctx->getSignature().isNull());
    CPPUNIT_ASSERT_EQUAL(std::string("pgp"), dctx->getSignature()->getType());
  }
  // second file
  {
    SharedHandle<RequestGroup> rg = groups[1];
    std::vector<std::string> uris;
    rg->getDownloadContext()->getFirstFileEntry()->getUris(uris);
    CPPUNIT_ASSERT_EQUAL((size_t)2, uris.size());

    const SharedHandle<DownloadContext>& dctx = rg->getDownloadContext();

    CPPUNIT_ASSERT(!dctx.isNull());
    CPPUNIT_ASSERT_EQUAL(std::string("/tmp"), dctx->getDir());
#ifdef ENABLE_MESSAGE_DIGEST
    CPPUNIT_ASSERT_EQUAL(MessageDigestContext::SHA1, dctx->getPieceHashAlgo());
    CPPUNIT_ASSERT_EQUAL((size_t)2, dctx->getPieceHashes().size());
    CPPUNIT_ASSERT_EQUAL((size_t)262144, dctx->getPieceLength());
    CPPUNIT_ASSERT_EQUAL(MessageDigestContext::SHA1,
                         dctx->getChecksumHashAlgo());
    CPPUNIT_ASSERT_EQUAL
      (std::string("4c255b0ed130f5ea880f0aa061c3da0487e251cc"),
       dctx->getChecksum());
#endif // ENABLE_MESSAGE_DIGEST
    CPPUNIT_ASSERT(dctx->getSignature().isNull());
  }

#ifdef ENABLE_BITTORRENT
  // fifth file <- downloading .torrent file
  {
    SharedHandle<RequestGroup> rg = groups[4];
    std::vector<std::string> uris;
    rg->getDownloadContext()->getFirstFileEntry()->getUris(uris);
    CPPUNIT_ASSERT_EQUAL((size_t)1, uris.size());
    CPPUNIT_ASSERT_EQUAL
      (std::string("http://host/torrent-http.integrated.torrent"), uris[0]);
    const SharedHandle<DownloadContext>& dctx = rg->getDownloadContext();

    CPPUNIT_ASSERT(!dctx.isNull());
    CPPUNIT_ASSERT_EQUAL(groups[5]->getGID(), rg->belongsTo());
  }
#endif // ENABLE_BITTORRENT

  // sixth file <- depends on fifth file to download .torrent file.
  {
#ifdef ENABLE_BITTORRENT
    SharedHandle<RequestGroup> rg = groups[5];
#else
    SharedHandle<RequestGroup> rg = groups[4];
#endif // ENABLE_BITTORRENT
    std::vector<std::string> uris;
    rg->getDownloadContext()->getFirstFileEntry()->getUris(uris);
    CPPUNIT_ASSERT_EQUAL((size_t)1, uris.size());
    CPPUNIT_ASSERT_EQUAL
      (std::string("http://host/torrent-http.integrated"), uris[0]);

    const SharedHandle<DownloadContext>& dctx = rg->getDownloadContext();

    CPPUNIT_ASSERT(!dctx.isNull());
    CPPUNIT_ASSERT_EQUAL(std::string("/tmp"), dctx->getDir());
  }
}

void Metalink2RequestGroupTest::testGenerate_groupByMetaurl()
{
  std::vector<SharedHandle<RequestGroup> > groups;
  Metalink2RequestGroup().generate(groups, "metalink4-groupbymetaurl.xml",
                                   option_);
  CPPUNIT_ASSERT_EQUAL((size_t)3, groups.size());

#ifdef ENABLE_BITTORRENT
  // first RequestGroup is torrent for second RequestGroup
  {
    SharedHandle<RequestGroup> rg = groups[0];
    std::vector<std::string> uris;
    rg->getDownloadContext()->getFirstFileEntry()->getUris(uris);
    CPPUNIT_ASSERT_EQUAL((size_t)1, uris.size());
    CPPUNIT_ASSERT_EQUAL(std::string("http://torrent"), uris[0]);
  }
  // second
  {
    SharedHandle<RequestGroup> rg = groups[1];
    SharedHandle<DownloadContext> dctx = rg->getDownloadContext();
    const std::vector<SharedHandle<FileEntry> >& fileEntries =
      dctx->getFileEntries();
    CPPUNIT_ASSERT_EQUAL((size_t)2, fileEntries.size());
    CPPUNIT_ASSERT_EQUAL(std::string("./file1"), fileEntries[0]->getPath());
    CPPUNIT_ASSERT_EQUAL(std::string("file1"), fileEntries[0]->getOriginalName());
    CPPUNIT_ASSERT_EQUAL((size_t)1, fileEntries[0]->getRemainingUris().size());
    CPPUNIT_ASSERT_EQUAL(std::string("http://file1p1"),
                         fileEntries[0]->getRemainingUris()[0]);
    CPPUNIT_ASSERT_EQUAL(std::string("./file3"), fileEntries[1]->getPath());
    CPPUNIT_ASSERT_EQUAL(std::string("file3"), fileEntries[1]->getOriginalName());
    CPPUNIT_ASSERT_EQUAL((size_t)1, fileEntries[1]->getRemainingUris().size());
    CPPUNIT_ASSERT_EQUAL(std::string("http://file3p1"),
                         fileEntries[1]->getRemainingUris()[0]);
  }
  // third
  {
    SharedHandle<RequestGroup> rg = groups[2];
    SharedHandle<DownloadContext> dctx = rg->getDownloadContext();
    const std::vector<SharedHandle<FileEntry> >& fileEntries =
      dctx->getFileEntries();
    CPPUNIT_ASSERT_EQUAL((size_t)1, fileEntries.size());
    CPPUNIT_ASSERT_EQUAL(std::string("./file2"), fileEntries[0]->getPath());
    CPPUNIT_ASSERT_EQUAL((size_t)1, fileEntries[0]->getRemainingUris().size());
    CPPUNIT_ASSERT_EQUAL(std::string("http://file2p1"),
                         fileEntries[0]->getRemainingUris()[0]);
  }
#else // !ENABLE_BITTORRENT
  {
    SharedHandle<RequestGroup> rg = groups[0];
    std::vector<std::string> uris;
    rg->getDownloadContext()->getFirstFileEntry()->getUris(uris);
    CPPUNIT_ASSERT_EQUAL((size_t)1, uris.size());
    CPPUNIT_ASSERT_EQUAL(std::string("http://file1p1"), uris[0]);
  }
  {
    SharedHandle<RequestGroup> rg = groups[1];
    std::vector<std::string> uris;
    rg->getDownloadContext()->getFirstFileEntry()->getUris(uris);
    CPPUNIT_ASSERT_EQUAL((size_t)1, uris.size());
    CPPUNIT_ASSERT_EQUAL(std::string("http://file2p1"), uris[0]);
  }
  {
    SharedHandle<RequestGroup> rg = groups[2];
    std::vector<std::string> uris;
    rg->getDownloadContext()->getFirstFileEntry()->getUris(uris);
    CPPUNIT_ASSERT_EQUAL((size_t)1, uris.size());
    CPPUNIT_ASSERT_EQUAL(std::string("http://file3p1"), uris[0]);
  }
  
#endif // !ENABLE_BITTORRENT
}

void Metalink2RequestGroupTest::testGenerate_dosDirTraversal()
{
#ifdef __MINGW32__
#ifdef ENABLE_BITTORRENT
  std::vector<SharedHandle<RequestGroup> > groups;
  option_->put(PREF_DIR, "/tmp");
  Metalink2RequestGroup().generate
    (groups, "metalink4-dosdirtraversal.xml", option_);
  CPPUNIT_ASSERT_EQUAL((size_t)3, groups.size());
  SharedHandle<RequestGroup> rg = groups[0];
  SharedHandle<FileEntry> file = rg->getDownloadContext()->getFirstFileEntry();
  CPPUNIT_ASSERT_EQUAL(std::string("/tmp/.._.._example.ext"),
                       file->getPath());
  
  rg = groups[2];
  file = rg->getDownloadContext()->getFileEntries()[0];
  CPPUNIT_ASSERT_EQUAL(std::string("/tmp/.._.._file1.ext"),
                       file->getPath());
  file = rg->getDownloadContext()->getFileEntries()[1];
  CPPUNIT_ASSERT_EQUAL(std::string("/tmp/.._.._file2.ext"),
                       file->getPath());
#endif // ENABLE_BITTORRENT
#endif // __MINGW32__
}

} // namespace aria2
