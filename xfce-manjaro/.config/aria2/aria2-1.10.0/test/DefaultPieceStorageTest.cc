#include "DefaultPieceStorage.h"

#include <cppunit/extensions/HelperMacros.h>

#include "util.h"
#include "Exception.h"
#include "Piece.h"
#include "Peer.h"
#include "Option.h"
#include "FileEntry.h"
#include "RarestPieceSelector.h"
#include "InOrderPieceSelector.h"
#include "DownloadContext.h"
#include "bittorrent_helper.h"

namespace aria2 {

class DefaultPieceStorageTest:public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(DefaultPieceStorageTest);
  CPPUNIT_TEST(testGetTotalLength);
  CPPUNIT_TEST(testGetMissingPiece);
  CPPUNIT_TEST(testGetMissingPiece_excludedIndexes);
  CPPUNIT_TEST(testGetMissingFastPiece);
  CPPUNIT_TEST(testGetMissingFastPiece_excludedIndexes);
  CPPUNIT_TEST(testHasMissingPiece);
  CPPUNIT_TEST(testCompletePiece);
  CPPUNIT_TEST(testGetPiece);
  CPPUNIT_TEST(testGetPieceInUsedPieces);
  CPPUNIT_TEST(testGetPieceCompletedPiece);
  CPPUNIT_TEST(testCancelPiece);
  CPPUNIT_TEST(testMarkPiecesDone);
  CPPUNIT_TEST(testGetCompletedLength);
  CPPUNIT_TEST(testGetNextUsedIndex);
  CPPUNIT_TEST_SUITE_END();
private:
  SharedHandle<DownloadContext> dctx_;
  SharedHandle<Peer> peer;
  Option* option;
  SharedHandle<PieceSelector> pieceSelector_;
public:
  void setUp() {
    dctx_.reset(new DownloadContext());
    bittorrent::load("test.torrent", dctx_);
    peer.reset(new Peer("192.168.0.1", 6889));
    peer->allocateSessionResource(dctx_->getPieceLength(),
                                  dctx_->getTotalLength());
    option = new Option();
    pieceSelector_.reset(new InOrderPieceSelector());
  }

  void tearDown()
  {
    delete option;
    option = 0;
  }

  void testGetTotalLength();
  void testGetMissingPiece();
  void testGetMissingPiece_excludedIndexes();
  void testGetMissingFastPiece();
  void testGetMissingFastPiece_excludedIndexes();
  void testHasMissingPiece();
  void testCompletePiece();
  void testGetPiece();
  void testGetPieceInUsedPieces();
  void testGetPieceCompletedPiece();
  void testCancelPiece();
  void testMarkPiecesDone();
  void testGetCompletedLength();
  void testGetNextUsedIndex();
};


CPPUNIT_TEST_SUITE_REGISTRATION(DefaultPieceStorageTest);

void DefaultPieceStorageTest::testGetTotalLength() {
  DefaultPieceStorage pss(dctx_, option);

  CPPUNIT_ASSERT_EQUAL((uint64_t)384ULL, pss.getTotalLength());
}

void DefaultPieceStorageTest::testGetMissingPiece() {
  DefaultPieceStorage pss(dctx_, option);
  pss.setPieceSelector(pieceSelector_);
  pss.setEndGamePieceNum(0);

  peer->setAllBitfield();

  SharedHandle<Piece> piece = pss.getMissingPiece(peer);
  CPPUNIT_ASSERT_EQUAL(std::string("piece: index=0, length=128"),
                       piece->toString());
  piece = pss.getMissingPiece(peer);
  CPPUNIT_ASSERT_EQUAL(std::string("piece: index=1, length=128"),
                       piece->toString());
  piece = pss.getMissingPiece(peer);
  CPPUNIT_ASSERT_EQUAL(std::string("piece: index=2, length=128"),
                       piece->toString());
  piece = pss.getMissingPiece(peer);
  CPPUNIT_ASSERT(piece.isNull());
}

void DefaultPieceStorageTest::testGetMissingPiece_excludedIndexes()
{
  DefaultPieceStorage pss(dctx_, option);
  pss.setPieceSelector(pieceSelector_);
  pss.setEndGamePieceNum(0);

  peer->setAllBitfield();

  std::vector<size_t> excludedIndexes;
  excludedIndexes.push_back(1);

  SharedHandle<Piece> piece = pss.getMissingPiece(peer, excludedIndexes);
  CPPUNIT_ASSERT_EQUAL(std::string("piece: index=0, length=128"),
                       piece->toString());

  piece = pss.getMissingPiece(peer, excludedIndexes);
  CPPUNIT_ASSERT_EQUAL(std::string("piece: index=2, length=128"),
                       piece->toString());

  piece = pss.getMissingPiece(peer, excludedIndexes);
  CPPUNIT_ASSERT(piece.isNull());
}

void DefaultPieceStorageTest::testGetMissingFastPiece() {
  DefaultPieceStorage pss(dctx_, option);
  pss.setPieceSelector(pieceSelector_);
  pss.setEndGamePieceNum(0);

  peer->setAllBitfield();
  peer->setFastExtensionEnabled(true);
  peer->addPeerAllowedIndex(2);

  SharedHandle<Piece> piece = pss.getMissingFastPiece(peer);
  CPPUNIT_ASSERT_EQUAL(std::string("piece: index=2, length=128"),
                       piece->toString());

  CPPUNIT_ASSERT(pss.getMissingFastPiece(peer).isNull());
}

void DefaultPieceStorageTest::testGetMissingFastPiece_excludedIndexes()
{
  DefaultPieceStorage pss(dctx_, option);
  pss.setPieceSelector(pieceSelector_);
  pss.setEndGamePieceNum(0);

  peer->setAllBitfield();
  peer->setFastExtensionEnabled(true);
  peer->addPeerAllowedIndex(1);
  peer->addPeerAllowedIndex(2);

  std::vector<size_t> excludedIndexes;
  excludedIndexes.push_back(2);

  SharedHandle<Piece> piece = pss.getMissingFastPiece(peer, excludedIndexes);
  CPPUNIT_ASSERT_EQUAL(std::string("piece: index=1, length=128"),
                       piece->toString());
  
  CPPUNIT_ASSERT(pss.getMissingFastPiece(peer, excludedIndexes).isNull());
}

void DefaultPieceStorageTest::testHasMissingPiece() {
  DefaultPieceStorage pss(dctx_, option);

  CPPUNIT_ASSERT(!pss.hasMissingPiece(peer));
  
  peer->setAllBitfield();

  CPPUNIT_ASSERT(pss.hasMissingPiece(peer));
}

void DefaultPieceStorageTest::testCompletePiece() {
  DefaultPieceStorage pss(dctx_, option);
  pss.setPieceSelector(pieceSelector_);
  pss.setEndGamePieceNum(0);

  peer->setAllBitfield();

  SharedHandle<Piece> piece = pss.getMissingPiece(peer);
  CPPUNIT_ASSERT_EQUAL(std::string("piece: index=0, length=128"),
                       piece->toString());

  CPPUNIT_ASSERT_EQUAL((uint64_t)0ULL, pss.getCompletedLength());

  pss.completePiece(piece);

  CPPUNIT_ASSERT_EQUAL((uint64_t)128ULL, pss.getCompletedLength());

  SharedHandle<Piece> incompletePiece = pss.getMissingPiece(peer);
  incompletePiece->completeBlock(0);
  CPPUNIT_ASSERT_EQUAL((uint64_t)256ULL, pss.getCompletedLength());
}

void DefaultPieceStorageTest::testGetPiece() {
  DefaultPieceStorage pss(dctx_, option);
  
  SharedHandle<Piece> pieceGot = pss.getPiece(0);
  CPPUNIT_ASSERT_EQUAL((size_t)0, pieceGot->getIndex());
  CPPUNIT_ASSERT_EQUAL((size_t)128, pieceGot->getLength());
  CPPUNIT_ASSERT_EQUAL(false, pieceGot->pieceComplete());
}

void DefaultPieceStorageTest::testGetPieceInUsedPieces() {
  DefaultPieceStorage pss(dctx_, option);
  SharedHandle<Piece> piece = SharedHandle<Piece>(new Piece(0, 128));
  piece->completeBlock(0);
  pss.addUsedPiece(piece);
  SharedHandle<Piece> pieceGot = pss.getPiece(0);
  CPPUNIT_ASSERT_EQUAL((size_t)0, pieceGot->getIndex());
  CPPUNIT_ASSERT_EQUAL((size_t)128, pieceGot->getLength());
  CPPUNIT_ASSERT_EQUAL((size_t)1, pieceGot->countCompleteBlock());
}

void DefaultPieceStorageTest::testGetPieceCompletedPiece() {
  DefaultPieceStorage pss(dctx_, option);
  SharedHandle<Piece> piece = SharedHandle<Piece>(new Piece(0, 128));
  pss.completePiece(piece);
  SharedHandle<Piece> pieceGot = pss.getPiece(0);
  CPPUNIT_ASSERT_EQUAL((size_t)0, pieceGot->getIndex());
  CPPUNIT_ASSERT_EQUAL((size_t)128, pieceGot->getLength());
  CPPUNIT_ASSERT_EQUAL(true, pieceGot->pieceComplete());
}

void DefaultPieceStorageTest::testCancelPiece()
{
  size_t pieceLength = 256*1024;
  uint64_t totalLength = 32*pieceLength; // <-- make the number of piece greater than END_GAME_PIECE_NUM
  std::deque<std::string> uris1;
  uris1.push_back("http://localhost/src/file1.txt");
  SharedHandle<FileEntry> file1(new FileEntry("src/file1.txt", totalLength, 0 /*, uris1*/));

  SharedHandle<DownloadContext> dctx
    (new DownloadContext(pieceLength, totalLength, "src/file1.txt"));

  SharedHandle<DefaultPieceStorage> ps(new DefaultPieceStorage(dctx, option));

  SharedHandle<Piece> p = ps->getMissingPiece(0);
  p->completeBlock(0);
  
  ps->cancelPiece(p);

  SharedHandle<Piece> p2 = ps->getMissingPiece(0);

  CPPUNIT_ASSERT(p2->hasBlock(0));
}

void DefaultPieceStorageTest::testMarkPiecesDone()
{
  size_t pieceLength = 256*1024;
  uint64_t totalLength = 4*1024*1024;
  SharedHandle<DownloadContext> dctx
    (new DownloadContext(pieceLength, totalLength));

  DefaultPieceStorage ps(dctx, option);

  ps.markPiecesDone(pieceLength*10+16*1024*2+1);

  for(size_t i = 0; i < 10; ++i) {
    CPPUNIT_ASSERT(ps.hasPiece(i));
  }
  for(size_t i = 10; i < (totalLength+pieceLength-1)/pieceLength; ++i) {
    CPPUNIT_ASSERT(!ps.hasPiece(i));
  }
  CPPUNIT_ASSERT_EQUAL((uint64_t)pieceLength*10+16*1024*2, ps.getCompletedLength());

  ps.markPiecesDone(totalLength);

  for(size_t i = 0; i < (totalLength+pieceLength-1)/pieceLength; ++i) {
    CPPUNIT_ASSERT(ps.hasPiece(i));
  }

  ps.markPiecesDone(0);
  CPPUNIT_ASSERT_EQUAL((uint64_t)0, ps.getCompletedLength());
}

void DefaultPieceStorageTest::testGetCompletedLength()
{
  SharedHandle<DownloadContext> dctx
    (new DownloadContext(1024*1024, 256*1024*1024));
  
  DefaultPieceStorage ps(dctx, option);
  
  CPPUNIT_ASSERT_EQUAL((uint64_t)0, ps.getCompletedLength());

  ps.markPiecesDone(250*1024*1024);
  CPPUNIT_ASSERT_EQUAL((uint64_t)250*1024*1024, ps.getCompletedLength());

  std::vector<SharedHandle<Piece> > inFlightPieces;
  for(int i = 0; i < 2; ++i) {
    SharedHandle<Piece> p(new Piece(250+i, 1024*1024));
    for(int j = 0; j < 32; ++j) {
      p->completeBlock(j);
    }
    inFlightPieces.push_back(p);
    CPPUNIT_ASSERT_EQUAL((size_t)512*1024, p->getCompletedLength());
  }
  ps.addInFlightPiece(inFlightPieces);
  
  CPPUNIT_ASSERT_EQUAL((uint64_t)251*1024*1024, ps.getCompletedLength());

  ps.markPiecesDone(256*1024*1024);

  CPPUNIT_ASSERT_EQUAL((uint64_t)256*1024*1024, ps.getCompletedLength());
}

void DefaultPieceStorageTest::testGetNextUsedIndex()
{
  DefaultPieceStorage pss(dctx_, option);
  CPPUNIT_ASSERT_EQUAL((size_t)3, pss.getNextUsedIndex(0));
  SharedHandle<Piece> piece = pss.getMissingPiece(2);
  CPPUNIT_ASSERT_EQUAL((size_t)2, pss.getNextUsedIndex(0));
  pss.completePiece(piece);
  CPPUNIT_ASSERT_EQUAL((size_t)2, pss.getNextUsedIndex(0));
  piece = pss.getMissingPiece(0);
  CPPUNIT_ASSERT_EQUAL((size_t)2, pss.getNextUsedIndex(0));
}

} // namespace aria2
