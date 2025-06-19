/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

//==============================================================================
#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
    METHOD (getItemCount, "getItemCount", "()I") \
    METHOD (getItemAt, "getItemAt", "(I)Landroid/content/ClipData$Item;")

DECLARE_JNI_CLASS (AndroidClipData, "android/content/ClipData")
#undef JNI_CLASS_MEMBERS

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
    METHOD (getUri, "getUri", "()Landroid/net/Uri;")

DECLARE_JNI_CLASS (AndroidClipDataItem, "android/content/ClipData$Item")
#undef JNI_CLASS_MEMBERS

//==============================================================================
static StringArray createMimeTypes (const String& filters)
{
    StringArray mimeTypes;

    if (filters.isEmpty())
    {
        mimeTypes.add ("*/*");
        return mimeTypes;
    }

    StringArray extensions = StringArray::fromTokens (filters, ";,", String());

    for (const auto& ext : extensions)
    {
        String extension = ext.trim().toLowerCase();
        if (extension.startsWith ("*."))
            extension = extension.substring (2);
        else if (extension.startsWith ("*"))
            extension = extension.substring (1);
        else if (extension.startsWith ("."))
            extension = extension.substring (1);

        String mimeType;

        // Map common extensions to MIME types
        if (extension == "jpg" || extension == "jpeg")
            mimeType = "image/jpeg";
        else if (extension == "png")
            mimeType = "image/png";
        else if (extension == "gif")
            mimeType = "image/gif";
        else if (extension == "bmp")
            mimeType = "image/bmp";
        else if (extension == "webp")
            mimeType = "image/webp";
        else if (extension == "mp3")
            mimeType = "audio/mpeg";
        else if (extension == "wav")
            mimeType = "audio/wav";
        else if (extension == "ogg")
            mimeType = "audio/ogg";
        else if (extension == "mp4")
            mimeType = "video/mp4";
        else if (extension == "avi")
            mimeType = "video/x-msvideo";
        else if (extension == "mov" || extension == "qt")
            mimeType = "video/quicktime";
        else if (extension == "pdf")
            mimeType = "application/pdf";
        else if (extension == "txt")
            mimeType = "text/plain";
        else if (extension == "html" || extension == "htm")
            mimeType = "text/html";
        else if (extension == "xml")
            mimeType = "text/xml";
        else if (extension == "json")
            mimeType = "application/json";
        else if (extension == "zip")
            mimeType = "application/zip";
        else if (extension == "doc")
            mimeType = "application/msword";
        else if (extension == "docx")
            mimeType = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
        else if (extension == "xlsx")
            mimeType = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
        else if (extension == "pptx")
            mimeType = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
        else
            mimeType = "*/*"; // Fallback for unknown extensions

        if (mimeType.isNotEmpty() && ! mimeTypes.contains (mimeType))
            mimeTypes.add (mimeType);
    }

    if (mimeTypes.isEmpty())
        mimeTypes.add ("*/*");

    return mimeTypes;
}

//==============================================================================
class AndroidActivityCallback
{
public:
    AndroidActivityCallback (FileChooser::CompletionCallback cb)
        : callback (std::move (cb))
        , completed (false)
    {
    }

    void onActivityResult (int requestCode, int resultCode, LocalRef<jobject> data)
    {
        auto* env = getEnv();
        Array<File> results;

        if (resultCode == -1) // RESULT_OK
        {
            if (data.get() != nullptr)
            {
                // Handle single file selection
                LocalRef<jobject> uri (env->CallObjectMethod (data.get(), AndroidIntent.getData));

                if (uri.get() != nullptr)
                {
                    LocalRef<jstring> uriString ((jstring) env->CallObjectMethod (uri.get(), AndroidUri.toString));
                    String pathString = yupString (uriString.get());

                    // Try to get a real file path, or use the content URI
                    File resultFile = AndroidContentUriResolver::getLocalFileFromContentUri (URL (pathString));
                    if (resultFile == File())
                        resultFile = File (pathString); // Use the URI as-is

                    results.add (resultFile);
                }

                // Handle multiple file selection
                LocalRef<jobject> clipData (env->CallObjectMethod (data.get(), AndroidIntent.getClipData));
                if (clipData.get() != nullptr)
                {
                    jint itemCount = env->CallIntMethod (clipData.get(), AndroidClipData.getItemCount);

                    for (jint i = 0; i < itemCount; ++i)
                    {
                        LocalRef<jobject> item (env->CallObjectMethod (clipData.get(), AndroidClipData.getItemAt, i));
                        if (item.get() != nullptr)
                        {
                            LocalRef<jobject> itemUri (env->CallObjectMethod (item.get(), AndroidClipDataItem.getUri));
                            if (itemUri.get() != nullptr)
                            {
                                LocalRef<jstring> itemUriString ((jstring) env->CallObjectMethod (itemUri.get(), AndroidUri.toString));
                                String itemPathString = yupString (itemUriString.get());

                                File resultFile = AndroidContentUriResolver::getLocalFileFromContentUri (URL (itemPathString));
                                if (resultFile == File())
                                    resultFile = File (itemPathString);

                                results.add (resultFile);
                            }
                        }
                    }
                }
            }
        }

        // Invoke callback with results
        if (callback)
        {
            callback (resultCode == -1, results);
        }

        completed = true;
    }

    bool isCompleted() const { return completed; }

private:
    FileChooser::CompletionCallback callback;
    bool completed;
};


class FileChooser::FileChooserImpl : public AndroidActivityCallback
{
public:
    using AndroidActivityCallback::AndroidActivityCallback;
};

static AndroidActivityCallback* currentCallback = nullptr;

//==============================================================================
extern "C" JNIEXPORT void JNICALL
    Java_com_yourpackage_FileChooserActivity_onActivityResult (JNIEnv* env, jobject thiz, jint requestCode, jint resultCode, jobject data)
{
    if (currentCallback != nullptr)
    {
        LocalRef<jobject> dataRef (data);
        currentCallback->onActivityResult (requestCode, resultCode, dataRef);
    }
}

//==============================================================================
void FileChooser::showPlatformDialog (CompletionCallback callback, int flags)
{
    const bool isSave = (flags & saveMode) != 0;
    const bool canChooseFiles = (flags & canSelectFiles) != 0;
    const bool canChooseDirectories = (flags & canSelectDirectories) != 0;
    const bool allowsMultiple = (flags & canSelectMultipleItems) != 0;

    auto* env = getEnv();
    if (env == nullptr)
    {
        callback (false, {});
        return;
    }

    // Create the callback
    impl = std::make_unique<FileChooserImpl> (std::move (callback));
    currentCallback = impl.get();

    LocalRef<jobject> intent;

    if (isSave)
    {
        // Use ACTION_CREATE_DOCUMENT for save operations
        LocalRef<jstring> action (javaString ("android.intent.action.CREATE_DOCUMENT"));
        intent = LocalRef<jobject> (env->NewObject (AndroidIntent, AndroidIntent.constructWithString, action.get()));

        env->CallObjectMethod (intent.get(), AndroidIntent.addCategory, javaString ("android.intent.category.OPENABLE").get());

        // Set MIME type
        StringArray mimeTypes = createMimeTypes (filters);
        if (mimeTypes.size() > 0)
            env->CallObjectMethod (intent.get(), AndroidIntent.setType, javaString (mimeTypes[0]).get());

        // Set initial filename
        if (startingFile.getFileName().isNotEmpty())
            env->CallObjectMethod (intent.get(), AndroidIntent.putExtra, javaString ("android.intent.extra.TITLE").get(), javaString (startingFile.getFileName()).get());
    }
    else if (canChooseDirectories && ! canChooseFiles)
    {
        // Use ACTION_OPEN_DOCUMENT_TREE for directory selection
        LocalRef<jstring> action (javaString ("android.intent.action.OPEN_DOCUMENT_TREE"));
        intent = LocalRef<jobject> (env->NewObject (AndroidIntent, AndroidIntent.constructWithString, action.get()));
    }
    else
    {
        // Use ACTION_OPEN_DOCUMENT for file selection
        LocalRef<jstring> action (javaString ("android.intent.action.OPEN_DOCUMENT"));
        intent = LocalRef<jobject> (env->NewObject (AndroidIntent, AndroidIntent.constructWithString, action.get()));

        env->CallObjectMethod (intent.get(), AndroidIntent.addCategory, javaString ("android.intent.category.OPENABLE").get());

        // Enable multiple selection if requested
        if (allowsMultiple)
            env->CallObjectMethod (intent.get(), AndroidIntent.putExtra, javaString ("android.intent.extra.ALLOW_MULTIPLE").get(), true);

        // Set MIME types
        StringArray mimeTypes = createMimeTypes (filters);
        if (mimeTypes.size() == 1)
        {
            env->CallObjectMethod (intent.get(), AndroidIntent.setType, javaString (mimeTypes[0]).get());
        }
        else if (mimeTypes.size() > 1)
        {
            env->CallObjectMethod (intent.get(), AndroidIntent.setType, javaString ("*/*").get());

            LocalRef<jobjectArray> mimeTypeArray (env->NewObjectArray (mimeTypes.size(), JavaString, nullptr));
            for (int i = 0; i < mimeTypes.size(); ++i)
                env->SetObjectArrayElement (mimeTypeArray.get(), i, javaString (mimeTypes[i]).get());

            env->CallObjectMethod (intent.get(), AndroidIntent.putExtra, javaString ("android.intent.extra.MIME_TYPES").get(), mimeTypeArray.get());
        }
    }

    if (intent.get() != nullptr)
    {
        // Set title if provided
        if (title.isNotEmpty())
            env->CallObjectMethod (intent.get(), AndroidIntent.putExtra, javaString ("android.intent.extra.TITLE").get(), javaString (title).get());

        // Start the activity
        const int requestCode = 12345;
        env->CallVoidMethod (getMainActivity(), AndroidActivity.startActivityForResult, intent.get(), requestCode);

        // Wait for the result (simplified approach)
        auto startTime = Time::getCurrentTime();
        while (currentCallback != nullptr && ! currentCallback->isCompleted() && (Time::getCurrentTime() - startTime).inSeconds() < 30.0)
        {
            Thread::sleep (100);
        }
    }

    currentCallback = nullptr;
}

} // namespace yup
