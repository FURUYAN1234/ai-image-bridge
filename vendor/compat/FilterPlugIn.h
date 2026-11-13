// ABI subset of upstream FilterPlugIn.h; see PROVENANCE.md and LICENSE.
/**
 * @file FilterPlugIn.h
 * @author 青猫 (AonekoSS)
 * @brief フィルタープラグインAPI
 */
/**
 * @file FilterPlugIn.h
 * @author consome hollywood
 * @brief  isRectEmpty、intersectRects、addressOffset、BlendFunction の宣言を追加
 */

#pragma once
#include <functional>
#include <string>

namespace FilterPlugIn {

/// @note アラインメントとか気にするエリア
#pragma pack(push,1)
	extern "C" {
		typedef	unsigned char	Bool;
		typedef	char			Char;
		typedef	unsigned short	UniChar;
		typedef	unsigned char	UInt8;
		typedef	long int		Int;
		typedef	long long int	Int64;
		typedef	float			Float;
		typedef	double			Double;
		typedef	void* Ptr;

		//	座標
		struct Point { Int x, y; };

		//	サイズ
		struct Size { Int width, height; };

		//	矩形
		struct Rect { Int left, top, right, bottom; };

		//	RGBカラー
		struct RGBColor { UInt8 red, green, blue; };

		//	CMYKカラー
		struct CMYKColor { UInt8 cyan, magenta, yellow, keyplate; };

		//	ホスト
		struct _Host;
		typedef	struct _Host* HostObject;

		//	文字列
		struct	_String;
		typedef	struct _String* StringObject;

		//	ビットマップ
		struct _Bitmap;
		typedef	struct _Bitmap* BitmapObject;

		//	オフスクリーン
		struct _Offscreen;
		typedef	struct _Offscreen* OffscreenObject;

		//	プロパティ
		struct	_Property;
		typedef	struct _Property* PropertyObject;

		enum class CallResult : Int {
			Success = 0,
			Failed = -1,
		};

		enum class Selector : Int {
			ModuleInitialize = 0x0101,
			ModuleTerminate = 0x0102,
			FilterInitialize = 0x0201,
			FilterRun = 0x0202,
			FilterTerminate = 0x0203,
		};

		// プロパティコールバック
		enum class PropertyCallBackNotify : Int {
			ValueChanged = 0x11,
			ButtonPushed = 0x21,
			ValueCheck = 0x31,
		};
		enum class PropertyCallBackResult : Int {
			NoModify = 0x01,
			Modify = 0x02,
			Invalid = 0x03,
		};
		typedef void (*PropertyCallBackProc)(PropertyCallBackResult* result, PropertyObject propertyObject, const Int itemKey, const PropertyCallBackNotify notify, Ptr data);

		// モジュール初期化レコード
		struct ModuleInitializeRecord {
			Int(*getHostVersionProc)(Int* hostVersion, HostObject hostObject);
			Int(*setModuleIDProc)(HostObject hostObject, StringObject moduleID);
			Int(*setModuleKindProc)(HostObject hostObject, const Int moduleKind);
		};

		//	フィルタ初期化レコード
		struct FilterInitializeRecord {
			Int(*setFilterCategoryNameProc)(HostObject hostObject, StringObject filterCategoryName, const Char accessKey);
			Int(*setFilterNameProc)(HostObject hostObject, StringObject filterName, const Char accessKey);
			Int(*setCanPreviewProc)(HostObject hostObject, const Bool canPreview);
			Int(*setUseBlankImageProc)(HostObject hostObject, const Bool useBlankImage);
			Int(*setTargetKindsProc)(HostObject hostObject, const Int* targetKinds, const Int targetKindCount);
			Int(*setPropertyProc)(HostObject hostObject, PropertyObject propertyObject);
			Int(*setPropertyCallBackProc)(HostObject hostObject, PropertyCallBackProc propertyCallBackProc, Ptr data);
		};

		//	フィルタ実行レコード
		struct FilterRunRecord {
			Int(*getPropertyProc)(PropertyObject* propertyObject, HostObject hostObject);
			void* dummy1[4];
            // AI Image Bridge extension: identified against local official SDK, not upstream.
            Int(*isLayerMaskSelectedProc)(Bool* selected, HostObject hostObject);
			Int(*isAlphaLockedProc)(Bool* locked, HostObject hostObject);
			Int(*getSourceOffscreenProc)(OffscreenObject* offscreenObject, HostObject hostObject);
			Int(*getDestinationOffscreenProc)(OffscreenObject* offscreenObject, HostObject hostObject);
			Int(*getSelectAreaRectProc)(Rect* seclectAreaRect, HostObject hostObject);
			// AI Image Bridge extension: selection presence query.
            Int(*hasSelectAreaOffscreenProc)(Bool* selected, HostObject hostObject);
			Int(*getSelectAreaOffscreenProc)(OffscreenObject* offscreenObject, HostObject hostObject);
			Int(*updateDestinationOffscreenRectProc)(HostObject hostObject, const Rect* updateRect);
			void* dummy3[3];
			Int(*processProc)(Int* result, HostObject hostObject, const Int processState);
			Int(*setProgressTotalProc)(HostObject hostObject, const Int progressTotal);
			Int(*setProgressDoneProc)(HostObject hostObject, const Int progressDone);
		};

		//	文字列サービス
		struct StringService {
			Int(*createWithAsciiStringProc)(StringObject* stringObject, const Char* ascii, const Int length);
			Int(*createWithUnicodeStringProc)(StringObject* stringObject, const UniChar* unicode, const Int length);
			Int(*createWithLocalCodeStringProc)(StringObject* stringObject, const Char* localcode, const Int length);
			Int(*createWithStringIDProc)(StringObject* stringObject, const Int stringID, HostObject hostObject);
			Int(*retainProc)(StringObject stringObject);
			Int(*releaseProc)(StringObject stringObject);
			Int(*getUnicodeCharsProc)(const UniChar** unicode, StringObject stringObject);
			Int(*getUnicodeLengthProc)(Int* length, StringObject stringObject);
			Int(*getLocalCodeCharsProc)(const Char** localcode, StringObject stringObject);
			Int(*getLocalCodeLengthProc)(Int* length, StringObject stringObject);
		};

		//	ビットマップサービス
		struct BitmapService {
			Int(*createProc)(BitmapObject* bitmapObject, const Int width, const Int height, const Int depth, const Int scanline);
			Int(*retainProc)(BitmapObject bitmapObject);
			Int(*releaseProc)(BitmapObject bitmapObject);
			Int(*getWidthProc)(Int* width, BitmapObject bitmapObject);
			Int(*getHeightProc)(Int* height, BitmapObject bitmapObject);
			Int(*getDepthProc)(Int* depth, BitmapObject bitmapObject);
			Int(*getScanlineProc)(Int* scanline, BitmapObject bitmapObject);
			Int(*getAddressProc)(Ptr* address, BitmapObject bitmapObject, const Point* pos);
			Int(*getRowBytesProc)(Int* rowBytes, BitmapObject bitmapObject);
			Int(*getPixelBytesProc)(Int* pixelBytes, BitmapObject bitmapObject);
		};

		//	オフスクリーンサービス
		struct OffscreenService {
			Int(*createPlaneProc)(OffscreenObject* offscreenObject, const Int width, const Int height, const Int depth);
			Int(*retainProc)(OffscreenObject offscreenObject);
			Int(*releaseProc)(OffscreenObject offscreenObject);
			Int(*getWidthProc)(Int* width, OffscreenObject offscreenObject);
			Int(*getHeightProc)(Int* height, OffscreenObject offscreenObject);
			Int(*getRectProc)(Rect* rect, OffscreenObject offscreenObject);
			Int(*getExtentRectProc)(Rect* rect, OffscreenObject offscreenObject);
			Int(*getChannelOrderProc)(Int* channelOrder, OffscreenObject offscreenObject);
			Int(*getRGBChannelIndexProc)(Int* redChannelIndex, Int* greenChannelIndex, Int* blueChannelIndex, OffscreenObject offscreenObject);
			Int(*getCMYKChannelIndexProc)(Int* cyanChannelIndex, Int* magentaChannelIndex, Int* yellowChannelIndex, Int* keytoneChannelIndex, OffscreenObject offscreenObject);
			Int(*getBlockRectCountProc)(Int* blockRectCount, OffscreenObject offscreenObject, Rect* bounds);
			Int(*getBlockRectProc)(Rect* blockRect, Int blockIndex, OffscreenObject offscreenObject, Rect* bounds);
			Int(*getBlockImageProc)(Ptr* address, Int* rowBytes, Int* pixelBytes, Rect* blockRect, OffscreenObject offscreenObject, Point* pos);
			Int(*getBlockAlphaProc)(Ptr* address, Int* rowBytes, Int* pixelBytes, Rect* blockRect, OffscreenObject offscreenObject, Point* pos);
			Int(*getBlockSelectAreaProc)(Ptr* address, Int* rowBytes, Int* pixelBytes, Rect* blockRect, OffscreenObject offscreenObject, Point* pos);
			Int(*getBlockPlaneProc)(Ptr* address, Int* rowBytes, Int* pixelBytes, Rect* blockRect, OffscreenObject offscreenObject, Point* pos);
			Int(*getTileWidthProc)(Int* tileWidth, OffscreenObject offscreenObject);
			Int(*getTileHeightProc)(Int* tileHeight, OffscreenObject offscreenObject);
			Int(*getBitmapProc)(BitmapObject bitmapObject, const Point* bitmapPos, OffscreenObject offscreenObject, const Point* offscreenPos, const Int copyWidth, const Int copyHeight, const Int copyMode);
			Int(*setBitmapProc)(OffscreenObject offscreenObject, const Point* offscreenPos, BitmapObject bitmapObject, const Point* bitmapPos, const Int copyWidth, const Int copyHeight, const Int copyMode);
		};

		//	オフスクリーンサービス2
		struct OffscreenService2 {
			Int(*getBitmapNormalAlphaChannelIndexProc)(Int* alphaChannelIndex, OffscreenObject offscreenObject);
		};

		//	プロパティサービス
		struct PropertyService {
			Int(*createProc)(PropertyObject* propertyObject);
			Int(*retainProc)(PropertyObject propertyObject);
			Int(*releaseProc)(PropertyObject propertyObject);
			Int(*addItemProc)(PropertyObject propertyObject, const Int itemKey, const Int valueType, const Int valueKind, const Int inputKind, StringObject caption, const Char accessKey);
			Int(*setBooleanValueProc)(PropertyObject propertyObject, const Int itemKey, const Bool value);
			Int(*getBooleanValueProc)(Bool* value, PropertyObject propertyObject, const Int itemKey);
			Int(*setBooleanDefaultValueProc)(PropertyObject propertyObject, const Int itemKey, const Bool defaultValue);
			Int(*getBooleanDefaultValueProc)(Bool* defaultValue, PropertyObject propertyObject, const Int itemKey);
			Int(*setIntegerValueProc)(PropertyObject propertyObject, const Int itemKey, const Int value);
			Int(*getIntegerValueProc)(Int* value, PropertyObject propertyObject, const Int itemKey);
			Int(*setIntegerDefaultValueProc)(PropertyObject propertyObject, const Int itemKey, const Int defaultValue);
			Int(*getIntegerDefaultValueProc)(Int* defaultValue, PropertyObject propertyObject, const Int itemKey);
			Int(*setIntegerMinValueProc)(PropertyObject propertyObject, const Int itemKey, const Int minValue);
			Int(*getIntegerMinValueProc)(Int* minValue, PropertyObject propertyObject, const Int itemKey);
			Int(*setIntegerMaxValueProc)(PropertyObject propertyObject, const Int itemKey, const Int maxValue);
			Int(*getIntegerMaxValueProc)(Int* maxValue, PropertyObject propertyObject, const Int itemKey);
			Int(*setDecimalValueProc)(PropertyObject propertyObject, const Int itemKey, const Double value);
			Int(*getDecimalValueProc)(Double* value, PropertyObject propertyObject, const Int itemKey);
			Int(*setDecimalDefaultValueProc)(PropertyObject propertyObject, const Int itemKey, const Double defaultValue);
			Int(*getDecimalDefaultValueProc)(Double* defaultValue, PropertyObject propertyObject, const Int itemKey);
			Int(*setDecimalMinValueProc)(PropertyObject propertyObject, const Int itemKey, const Double minValue);
			Int(*getDecimalMinValueProc)(Double* minValue, PropertyObject propertyObject, const Int itemKey);
			Int(*setDecimalMaxValueProc)(PropertyObject propertyObject, const Int itemKey, const Double maxValue);
			Int(*getDecimalMaxValueProc)(Double* maxValue, PropertyObject propertyObject, const Int itemKey);
		};

		//	プロパティサービス2
		struct PropertyService2 {
			Int(*setItemStoreValueProc)(PropertyObject propertyObject, const Int itemKey, const Bool storeValue);
			Int(*setPointValueProc)(PropertyObject propertyObject, const Int itemKey, const Point* value);
			Int(*getPointValueProc)(Point* value, PropertyObject propertyObject, const Int itemKey);
			Int(*setPointDefaultValueKindProc)(PropertyObject propertyObject, const Int itemKey, const Int defaultValueKind);
			Int(*getPointDefaultValueKindProc)(Int* defaultValueKind, PropertyObject propertyObject, const Int itemKey);
			Int(*setPointDefaultValueProc)(PropertyObject propertyObject, const Int itemKey, const Point* defaultValue);
			Int(*getPointDefaultValueProc)(Point* defaultValue, PropertyObject propertyObject, const Int itemKey);
			Int(*setPointMinMaxValueKindProc)(PropertyObject propertyObject, const Int itemKey, const Int minMaxValueKind);
			Int(*getPointMinMaxValueKindProc)(Int* minMaxValueKind, PropertyObject propertyObject, const Int itemKey);
			Int(*setPointMinValueProc)(PropertyObject propertyObject, const Int itemKey, const Point* minValue);
			Int(*getPointMinValueProc)(Point* minValue, PropertyObject propertyObject, const Int itemKey);
			Int(*setPointMaxValueProc)(PropertyObject propertyObject, const Int itemKey, const Point* maxValue);
			Int(*getPointMaxValueProc)(Point* maxValue, PropertyObject propertyObject, const Int itemKey);
			Int(*setEnumerationValueProc)(PropertyObject propertyObject, const Int itemKey, const Int value);
			Int(*getEnumerationValueProc)(Int* value, PropertyObject propertyObject, const Int itemKey);
			Int(*setEnumerationDefaultValueProc)(PropertyObject propertyObject, const Int itemKey, const Int value);
			Int(*getEnumerationDefaultValueProc)(Int* value, PropertyObject propertyObject, const Int itemKey);
			Int(*addEnumerationItemProc)(PropertyObject propertyObject, const Int itemKey, const Int value, StringObject caption, const Char accessKey);
			Int(*setStringValueProc)(PropertyObject propertyObject, const Int itemKey, StringObject value);
			Int(*getStringValueProc)(StringObject* value, PropertyObject propertyObject, const Int itemKey);
			Int(*setStringDefaultValueProc)(PropertyObject propertyObject, const Int itemKey, StringObject defaultValue);
			Int(*getStringDefaultValueProc)(StringObject* defaultValue, PropertyObject propertyObject, const Int itemKey);
			Int(*setStringMaxLengthProc)(PropertyObject propertyObject, const Int itemKey, const Int maxLength);
			Int(*getStringMaxLengthProc)(Int* maxLength, PropertyObject propertyObject, const Int itemKey);
		};

		// レコードスイート
		struct RecordSuite {
			ModuleInitializeRecord* moduleInitializeRecord;
			void* dumy1[4];
			FilterInitializeRecord* filterInitializeRecord;
			FilterRunRecord* filterRunRecord;
			void* dumy2[256 - 7];
		};

		//	サービススイート
		struct ServiceSuite {
			StringService* stringService;
			BitmapService* bitmapService;
			OffscreenService* offscreenService;
			void* dumy1[1];
			PropertyService* propertyService;
			void* dumy2[3];
			OffscreenService2* offscreenService2;
			PropertyService2* propertyService2;
			void* dumy3[256 - 10];
		};

		// サーバー
		struct Server {
			RecordSuite recordSuite;
			ServiceSuite serviceSuite;
			HostObject hostObject;
		};
	}
#pragma pack(pop)

} // namespace FilterPlugIn
