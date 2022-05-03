#include "PMXlsxImporterContextLogger.h"
#include "PMXlsxImporterLog.h"

FPMXlsxImporterContextLogger::FPMXlsxImporterContextLogger()
{
}

void FPMXlsxImporterContextLogger::Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category)
{
	FString Context = FString::Join(ContextStack, TEXT(""));
	Errors.Add(FString::Printf(TEXT("%s: %s"), *Context, V));
}

void FPMXlsxImporterContextLogger::Flush()
{
	for (const FString& Err : Errors)
	{
		UE_LOG(LogPMXlsxImporter, Error, TEXT("%s"), *Err);
	}
}

FPMXlsxImporterContextLoggerScopedContext FPMXlsxImporterContextLogger::PushContext(const FString& Context)
{
	ContextStack.Add(Context);
	return FPMXlsxImporterContextLoggerScopedContext(*this);
}

void FPMXlsxImporterContextLogger::PopContext()
{
	ContextStack.Pop();
}

int32 FPMXlsxImporterContextLogger::Num() const
{
	return Errors.Num();
}

FPMXlsxImporterContextLoggerScopedContext::FPMXlsxImporterContextLoggerScopedContext(FPMXlsxImporterContextLogger& Owner)
	: Owner(Owner)
{
}

FPMXlsxImporterContextLoggerScopedContext::~FPMXlsxImporterContextLoggerScopedContext()
{
	Owner.PopContext();
}
