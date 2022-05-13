// Copyright 2022 Proletariat, Inc.

#include "PMXlsxImporterStyle.h"
#include "PMXlsxImporter.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

#if ENGINE_MAJOR_VERSION == 5
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir
#endif

TSharedPtr<FSlateStyleSet> FPMXlsxImporterStyle::StyleInstance = nullptr;

void FPMXlsxImporterStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FPMXlsxImporterStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FPMXlsxImporterStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("PMXlsxImporterStyle"));
	return StyleSetName;
}

#if ENGINE_MAJOR_VERSION == 4
#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )
#endif

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

TSharedRef< FSlateStyleSet > FPMXlsxImporterStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("PMXlsxImporterStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("PMXlsxImporter")->GetBaseDir() / TEXT("Resources"));

#if ENGINE_MAJOR_VERSION == 4
	Style->Set("PMXlsxImporter.PluginAction", new IMAGE_BRUSH(TEXT("xlsxicon"), Icon40x40));
#elif ENGINE_MAJOR_VERSION == 5
	Style->Set("PMXlsxImporter.PluginAction", new IMAGE_BRUSH(TEXT("xlsxicon"), Icon20x20));
#else
#	error Unknown engine version
#endif
	return Style;
}

#if ENGINE_MAJOR_VERSION == 4
#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT
#endif

void FPMXlsxImporterStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FPMXlsxImporterStyle::Get()
{
	return *StyleInstance;
}
