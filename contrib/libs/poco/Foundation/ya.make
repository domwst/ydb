# Generated by devtools/yamaker.

LIBRARY()

OWNER(
    orivej
    g:cpp-contrib
)

LICENSE(
    BSD-3-Clause AND
    BSL-1.0 AND
    NCSA AND
    Public-Domain AND
    RSA-MD AND
    RSA-MD4
)

LICENSE_TEXTS(.yandex_meta/licenses.list.txt)

PEERDIR(
    contrib/libs/double-conversion
    contrib/libs/pcre
    contrib/libs/zlib
)

ADDINCL(
    GLOBAL contrib/libs/poco/Foundation/include
    contrib/libs/double-conversion
    contrib/libs/pcre
    contrib/libs/poco/Foundation/src
)

NO_COMPILER_WARNINGS()

NO_UTIL()

SRCS(
    src/ASCIIEncoding.cpp
    src/AbstractObserver.cpp
    src/ActiveDispatcher.cpp
    src/ArchiveStrategy.cpp
    src/Ascii.cpp
    src/AsyncChannel.cpp
    src/AtomicCounter.cpp
    src/Base32Decoder.cpp
    src/Base32Encoder.cpp
    src/Base64Decoder.cpp
    src/Base64Encoder.cpp
    src/BinaryReader.cpp
    src/BinaryWriter.cpp
    src/Bugcheck.cpp
    src/ByteOrder.cpp
    src/Channel.cpp
    src/Checksum.cpp
    src/Clock.cpp
    src/Condition.cpp
    src/Configurable.cpp
    src/ConsoleChannel.cpp
    src/CountingStream.cpp
    src/DateTime.cpp
    src/DateTimeFormat.cpp
    src/DateTimeFormatter.cpp
    src/DateTimeParser.cpp
    src/Debugger.cpp
    src/DeflatingStream.cpp
    src/DigestEngine.cpp
    src/DigestStream.cpp
    src/DirectoryIterator.cpp
    src/DirectoryIteratorStrategy.cpp
    src/DirectoryWatcher.cpp
    src/Environment.cpp
    src/Error.cpp
    src/ErrorHandler.cpp
    src/Event.cpp
    src/EventArgs.cpp
    src/EventChannel.cpp
    src/Exception.cpp
    src/FIFOBufferStream.cpp
    src/FPEnvironment.cpp
    src/File.cpp
    src/FileChannel.cpp
    src/FileStream.cpp
    src/FileStreamFactory.cpp
    src/Format.cpp
    src/Formatter.cpp
    src/FormattingChannel.cpp
    src/Glob.cpp
    src/Hash.cpp
    src/HashStatistic.cpp
    src/HexBinaryDecoder.cpp
    src/HexBinaryEncoder.cpp
    src/InflatingStream.cpp
    src/JSONString.cpp
    src/Latin1Encoding.cpp
    src/Latin2Encoding.cpp
    src/Latin9Encoding.cpp
    src/LineEndingConverter.cpp
    src/LocalDateTime.cpp
    src/LogFile.cpp
    src/LogStream.cpp
    src/Logger.cpp
    src/LoggingFactory.cpp
    src/LoggingRegistry.cpp
    src/MD4Engine.cpp
    src/MD5Engine.cpp
    src/Manifest.cpp
    src/MemoryPool.cpp
    src/MemoryStream.cpp
    src/Message.cpp
    src/Mutex.cpp
    src/NamedEvent.cpp
    src/NamedMutex.cpp
    src/NestedDiagnosticContext.cpp
    src/Notification.cpp
    src/NotificationCenter.cpp
    src/NotificationQueue.cpp
    src/NullChannel.cpp
    src/NullStream.cpp
    src/NumberFormatter.cpp
    src/NumberParser.cpp
    src/NumericString.cpp
    src/Path.cpp
    src/PatternFormatter.cpp
    src/Pipe.cpp
    src/PipeImpl.cpp
    src/PipeStream.cpp
    src/PriorityNotificationQueue.cpp
    src/Process.cpp
    src/PurgeStrategy.cpp
    src/RWLock.cpp
    src/Random.cpp
    src/RandomStream.cpp
    src/RefCountedObject.cpp
    src/RegularExpression.cpp
    src/RotateStrategy.cpp
    src/Runnable.cpp
    src/SHA1Engine.cpp
    src/Semaphore.cpp
    src/SharedLibrary.cpp
    src/SharedMemory.cpp
    src/SignalHandler.cpp
    src/SimpleFileChannel.cpp
    src/SortedDirectoryIterator.cpp
    src/SplitterChannel.cpp
    src/Stopwatch.cpp
    src/StreamChannel.cpp
    src/StreamConverter.cpp
    src/StreamCopier.cpp
    src/StreamTokenizer.cpp
    src/String.cpp
    src/StringTokenizer.cpp
    src/SynchronizedObject.cpp
    src/Task.cpp
    src/TaskManager.cpp
    src/TaskNotification.cpp
    src/TeeStream.cpp
    src/TemporaryFile.cpp
    src/TextBufferIterator.cpp
    src/TextConverter.cpp
    src/TextEncoding.cpp
    src/TextIterator.cpp
    src/Thread.cpp
    src/ThreadLocal.cpp
    src/ThreadPool.cpp
    src/ThreadTarget.cpp
    src/TimedNotificationQueue.cpp
    src/Timer.cpp
    src/Timespan.cpp
    src/Timestamp.cpp
    src/Timezone.cpp
    src/Token.cpp
    src/URI.cpp
    src/URIStreamFactory.cpp
    src/URIStreamOpener.cpp
    src/UTF16Encoding.cpp
    src/UTF32Encoding.cpp
    src/UTF8Encoding.cpp
    src/UTF8String.cpp
    src/UUID.cpp
    src/UUIDGenerator.cpp
    src/Unicode.cpp
    src/UnicodeConverter.cpp
    src/Var.cpp
    src/VarHolder.cpp
    src/VarIterator.cpp
    src/Void.cpp
    src/Windows1250Encoding.cpp
    src/Windows1251Encoding.cpp
    src/Windows1252Encoding.cpp
)

IF (OS_WINDOWS)
    SRCS(
        src/EventLogChannel.cpp
        src/WindowsConsoleChannel.cpp
    )
ELSE()
    SRCS(
        src/SyslogChannel.cpp
    )
ENDIF()

END()
