#pragma once

#include "CoreMinimal.h"

// In-memory collection of FStrings to be logged out later.
// Tracks context in a stack system and prepends that context to each log.
class PMXLSXIMPORTER_API FPMXlsxImporterContextLogger : public FOutputDevice
{
public:
	friend class FPMXlsxImporterContextLoggerScopedContext;

	FPMXlsxImporterContextLogger();

	// FOutputDevice interface
	// "Error" is the only verbosity used by this class. Category is ignored.
	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override;
	virtual void Flush() override;
	virtual bool IsMemoryOnly() const override
	{
		return true;
	}

	// Context is prepended to each log statement, from oldest to newest.
	// For example: PushContext("classname"); PushContext(".propertyname"); Log("foo");
	// will serialize "classname.propertyname foo"
	class FPMXlsxImporterContextLoggerScopedContext PushContext(const FString& Context);

	// Returns the number of errors that have been collected
	int32 Num() const;

private:
	void PopContext(); // Called when a ScopedContext falls out of scope

	TArray<FString> Errors;
	TArray<FString> ContextStack;
};

// Object that automatically pops a FPMXlsxImporterContextLogger's context when leaving scope
class PMXLSXIMPORTER_API FPMXlsxImporterContextLoggerScopedContext
{
public:
	FPMXlsxImporterContextLoggerScopedContext(FPMXlsxImporterContextLogger& Owner);
	~FPMXlsxImporterContextLoggerScopedContext();

private:
	FPMXlsxImporterContextLogger& Owner;
};
