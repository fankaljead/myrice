#include "IteratableChunkChecksumValidator.h"

#include <cppunit/extensions/HelperMacros.h>

#include "DownloadContext.h"
#include "DefaultPieceStorage.h"
#include "Option.h"
#include "DiskAdaptor.h"
#include "FileEntry.h"
#include "PieceSelector.h"
#include "messageDigest.h"

namespace aria2 {

class IteratableChunkChecksumValidatorTest:public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(IteratableChunkChecksumValidatorTest);
  CPPUNIT_TEST(testValidate);
  CPPUNIT_TEST(testValidate_readError);
  CPPUNIT_TEST_SUITE_END();
private:

  static const char* csArray[];
public:
  void setUp() {
  }

  void testValidate();
  void testValidate_readError();
};


CPPUNIT_TEST_SUITE_REGISTRATION( IteratableChunkChecksumValidatorTest );

const char* IteratableChunkChecksumValidatorTest::csArray[] = { "29b0e7878271645fffb7eec7db4a7473a1c00bc1",
                                                                "4df75a661cb7eb2733d9cdaa7f772eae3a4e2976",
                                                                "0a4ea2f7dd7c52ddf2099a444ab2184b4d341bdb" };

void IteratableChunkChecksumValidatorTest::testValidate() {
  Option option;
  SharedHandle<DownloadContext> dctx
    (new DownloadContext(100, 250, "chunkChecksumTestFile250.txt"));
  dctx->setPieceHashes(&csArray[0], &csArray[3]);
  dctx->setPieceHashAlgo(MessageDigestContext::SHA1);
  SharedHandle<DefaultPieceStorage> ps
    (new DefaultPieceStorage(dctx, &option));
  ps->initStorage();
  ps->getDiskAdaptor()->openFile();

  IteratableChunkChecksumValidator validator(dctx, ps);
  validator.init();

  validator.validateChunk();
  CPPUNIT_ASSERT(!validator.finished());
  validator.validateChunk();
  CPPUNIT_ASSERT(!validator.finished());
  validator.validateChunk();
  CPPUNIT_ASSERT(validator.finished());
  CPPUNIT_ASSERT(ps->downloadFinished());

  // make the test fail
  std::deque<std::string> badHashes(&csArray[0], &csArray[3]);
  badHashes[1] = "ffffffffffffffffffffffffffffffffffffffff";
  dctx->setPieceHashes(badHashes.begin(), badHashes.end());

  validator.init();

  while(!validator.finished()) {
    validator.validateChunk();
  }
  CPPUNIT_ASSERT(ps->hasPiece(0));
  CPPUNIT_ASSERT(!ps->hasPiece(1));
  CPPUNIT_ASSERT(ps->hasPiece(2));
}

void IteratableChunkChecksumValidatorTest::testValidate_readError() {
  Option option;
  SharedHandle<DownloadContext> dctx
    (new DownloadContext(100, 500, "chunkChecksumTestFile250.txt"));
  std::deque<std::string> hashes(&csArray[0], &csArray[3]);
  hashes.push_back("ffffffffffffffffffffffffffffffffffffffff");
  hashes.push_back("ffffffffffffffffffffffffffffffffffffffff");
  dctx->setPieceHashes(hashes.begin(), hashes.end());
  dctx->setPieceHashAlgo(MessageDigestContext::SHA1);
  SharedHandle<DefaultPieceStorage> ps(new DefaultPieceStorage(dctx, &option));
  ps->initStorage();
  ps->getDiskAdaptor()->openFile();

  IteratableChunkChecksumValidator validator(dctx, ps);
  validator.init();

  while(!validator.finished()) {
    validator.validateChunk();
  }

  CPPUNIT_ASSERT(ps->hasPiece(0));
  CPPUNIT_ASSERT(ps->hasPiece(1));
  CPPUNIT_ASSERT(!ps->hasPiece(2)); // #2 piece is not valid because
                                    // #program expects its size is
                                    // #100, but it reads only 50
                                    // #bytes and raises error.
  CPPUNIT_ASSERT(!ps->hasPiece(3));
  CPPUNIT_ASSERT(!ps->hasPiece(4));
}

} // namespace aria2
