TESTS = aria2c
check_PROGRAMS = $(TESTS)
aria2c_SOURCES = AllTest.cc\
	TestUtil.cc TestUtil.h\
	SocketCoreTest.cc\
	array_funTest.cc\
	Base64Test.cc\
	Base32Test.cc\
	SequenceTest.cc\
	a2functionalTest.cc\
	FileEntryTest.cc\
	PieceTest.cc\
	SegmentTest.cc\
	GrowSegmentTest.cc\
	SingleFileAllocationIteratorTest.cc\
	DefaultBtProgressInfoFileTest.cc\
	RequestGroupTest.cc\
	PStringBuildVisitorTest.cc\
	ParameterizedStringParserTest.cc\
	UtilTest.cc\
	AlphaNumberDecoratorTest.cc\
	UriListParserTest.cc\
	HttpHeaderProcessorTest.cc\
	RequestTest.cc\
	CookieParserTest.cc\
	HttpRequestTest.cc\
	RequestGroupManTest.cc\
	AuthConfigFactoryTest.cc\
	NetrcAuthResolverTest.cc\
	DefaultAuthResolverTest.cc\
	OptionHandlerTest.cc\
	SegmentManTest.cc\
	BitfieldManTest.cc\
	NetrcTest.cc\
	SingletonHolderTest.cc\
	HttpHeaderTest.cc\
	HttpResponseTest.cc\
	SharedHandleTest.cc\
	FileTest.cc\
	OptionTest.cc\
	DefaultDiskWriterTest.cc\
	FeatureConfigTest.cc\
	SpeedCalcTest.cc\
	MultiDiskAdaptorTest.cc\
	MultiFileAllocationIteratorTest.cc\
	FixedNumberRandomizer.h\
	ProtocolDetectorTest.cc\
	StringFormatTest.cc\
	ExceptionTest.cc\
	DownloadHandlerFactoryTest.cc\
	ChunkedDecoderTest.cc\
	SignatureTest.cc\
	ServerStatManTest.cc\
	FeedbackURISelectorTest.cc\
	InOrderURISelectorTest.cc\
	ServerStatTest.cc\
	NsCookieParserTest.cc\
	DirectDiskAdaptorTest.cc\
	CookieTest.cc\
	CookieStorageTest.cc\
	TimeTest.cc\
	FtpConnectionTest.cc\
	OptionParserTest.cc\
	DNSCacheTest.cc\
	DownloadHelperTest.cc\
	SequentialPickerTest.cc\
	RarestPieceSelectorTest.cc\
	PieceStatManTest.cc\
	InOrderPieceSelector.h\
	LongestSequencePieceSelectorTest.cc\
	a2algoTest.cc\
	bitfieldTest.cc\
	DownloadContextTest.cc\
	SessionSerializerTest.cc\
	ValueBaseTest.cc

if ENABLE_XML_RPC
aria2c_SOURCES += XmlRpcRequestParserControllerTest.cc\
	XmlRpcRequestProcessorTest.cc\
	XmlRpcMethodTest.cc
endif # ENABLE_XML_RPC

if HAVE_SOME_FALLOCATE
aria2c_SOURCES += FallocFileAllocationIteratorTest.cc
endif  # HAVE_SOME_FALLOCATE

if HAVE_LIBZ
aria2c_SOURCES += GZipDecoderTest.cc\
	GZipEncoderTest.cc
endif # HAVE_LIBZ

if HAVE_SQLITE3
aria2c_SOURCES += Sqlite3CookieParserTest.cc
endif # HAVE_SQLITE3

if ENABLE_MESSAGE_DIGEST
aria2c_SOURCES += MessageDigestHelperTest.cc\
	IteratableChunkChecksumValidatorTest.cc\
	IteratableChecksumValidatorTest.cc
endif # ENABLE_MESSAGE_DIGEST

if ENABLE_BITTORRENT
aria2c_SOURCES += BtAllowedFastMessageTest.cc\
	BtBitfieldMessageTest.cc\
	BtCancelMessageTest.cc\
	BtChokeMessageTest.cc\
	BtHandshakeMessageTest.cc\
	BtHaveAllMessageTest.cc\
	BtHaveMessageTest.cc\
	BtHaveNoneMessageTest.cc\
	BtInterestedMessageTest.cc\
	BtKeepAliveMessageTest.cc\
	BtNotInterestedMessageTest.cc\
	BtPieceMessageTest.cc\
	BtPortMessageTest.cc\
	BtRejectMessageTest.cc\
	BtRequestMessageTest.cc\
	BtSuggestPieceMessageTest.cc\
	BtUnchokeMessageTest.cc\
	DefaultPieceStorageTest.cc\
	DefaultBtAnnounceTest.cc\
	DefaultBtMessageDispatcherTest.cc\
	DefaultBtRequestFactoryTest.cc\
	MockBtMessage.h\
	MockBtMessageDispatcher.h\
	MockBtMessageFactory.h\
	AnnounceListTest.cc\
	DefaultPeerStorageTest.cc\
	MockPeerStorage.h\
	ByteArrayDiskWriterTest.cc\
	PeerTest.cc\
	PeerSessionResourceTest.cc\
	ShareRatioSeedCriteriaTest.cc\
	BtRegistryTest.cc\
	BtDependencyTest.cc\
	BtPostDownloadHandlerTest.cc\
	TimeSeedCriteriaTest.cc\
	BtExtendedMessageTest.cc\
	HandshakeExtensionMessageTest.cc\
	UTPexExtensionMessageTest.cc\
	UTMetadataRequestExtensionMessageTest.cc\
	UTMetadataDataExtensionMessageTest.cc\
	UTMetadataRejectExtensionMessageTest.cc\
	UTMetadataRequestTrackerTest.cc\
	UTMetadataRequestFactoryTest.cc\
	UTMetadataPostDownloadHandlerTest.cc\
	MagnetTest.cc\
	DefaultBtMessageFactoryTest.cc\
	DefaultExtensionMessageFactoryTest.cc\
	DHTNodeTest.cc\
	DHTBucketTest.cc\
	DHTRoutingTableTest.cc\
	DHTMessageTrackerEntryTest.cc\
	DHTMessageTrackerTest.cc\
	DHTConnectionImplTest.cc\
	DHTPingMessageTest.cc\
	DHTPingReplyMessageTest.cc\
	DHTFindNodeMessageTest.cc\
	DHTFindNodeReplyMessageTest.cc\
	DHTGetPeersMessageTest.cc\
	DHTGetPeersReplyMessageTest.cc\
	DHTAnnouncePeerMessageTest.cc\
	DHTAnnouncePeerReplyMessageTest.cc\
	DHTUnknownMessageTest.cc\
	DHTMessageFactoryImplTest.cc\
	BNodeTest.cc\
	DHTPeerAnnounceEntryTest.cc\
	DHTPeerAnnounceStorageTest.cc\
	DHTTokenTrackerTest.cc\
	XORCloserTest.cc\
	DHTIDCloserTest.cc\
	DHTRoutingTableSerializerTest.cc\
	DHTRoutingTableDeserializerTest.cc\
	DHKeyExchangeTest.cc\
	ARC4Test.cc\
	MSEHandshakeTest.cc\
	MockBtAnnounce.h\
	MockBtProgressInfoFile.h\
	MockBtRequestFactory.h\
	MockDHTMessage.h\
	MockDHTMessageCallback.h\
	MockDHTMessageDispatcher.h\
	MockDHTMessageFactory.h\
	MockDHTTask.h\
	MockDHTTaskFactory.h\
	MockDHTTaskQueue.h\
	MockExtensionMessage.h\
	MockExtensionMessageFactory.h\
	MockPieceStorage.h\
	BittorrentHelperTest.cc\
	PriorityPieceSelectorTest.cc\
	MockPieceSelector.h\
	extension_message_test_helper.h\
	LpdMessageDispatcherTest.cc\
	LpdMessageReceiverTest.cc\
	Bencode2Test.cc
endif # ENABLE_BITTORRENT

if ENABLE_METALINK
aria2c_SOURCES += MetalinkerTest.cc\
	MetalinkEntryTest.cc\
	Metalink2RequestGroupTest.cc\
	MetalinkPostDownloadHandlerTest.cc\
	MetalinkHelperTest.cc\
	MetalinkParserControllerTest.cc\
	MetalinkProcessorTest.cc
endif # ENABLE_METALINK

#aria2c_CXXFLAGS = ${CPPUNIT_CFLAGS} -I../src -I../lib -Wall -D_FILE_OFFSET_BITS=64
#aria2c_LDFLAGS = ${CPPUNIT_LIBS}

aria2c_LDADD = ../src/libaria2c.a\
    @LIBINTL@ @LIBGNUTLS_LIBS@\
	@LIBGCRYPT_LIBS@ @OPENSSL_LIBS@ @XML_LIBS@\
	@LIBCARES_LIBS@ @LIBEXPAT_LIBS@ @LIBZ_LIBS@\
	@SQLITE3_LIBS@\
	${CPPUNIT_LIBS}
AM_CPPFLAGS =  -Wall\
	${CPPUNIT_CFLAGS}\
	-I ../src\
	-I../lib -I../intl -I$(top_srcdir)/intl\
	@LIBGNUTLS_CFLAGS@ @LIBGCRYPT_CFLAGS@ @OPENSSL_CFLAGS@ @XML_CPPFLAGS@\
	@LIBCARES_CPPFLAGS@ @LIBEXPAT_CPPFLAGS@\
	@LIBZ_CPPFLAGS@	 @SQLITE3_CFLAGS@ -DLOCALEDIR=\"$(localedir)\" @DEFS@

EXTRA_DIST = 4096chunk.txt\
	chunkChecksumTestFile250.txt\
	cookies.sqlite\
	chromium_cookies.sqlite\
	emptyfile\
	file1r.txt\
	file2r.txt\
	file3r.txt\
	filelist1.txt\
	gzip_decode_test.gz\
	load-nonBt.aria2\
	load-nonBt-v0001.aria2\
	load.aria2\
	load-v0001.aria2\
	malformed.netrc\
	nscookietest.txt\
	sample.netrc\
	single.torrent\
	test.torrent\
	test.xml\
	url-list-multiFile.torrent\
	url-list-singleFile.torrent\
	url-list-singleFileEndsWithSlash.torrent\
	input_uris.txt\
	2files.metalink\
	utf8.torrent\
	metalink4.xml\
	metalink3-dirtraversal.xml\
	metalink4-groupbymetaurl.xml\
	serialize_session.meta4\
	metalink4-dosdirtraversal.xml
