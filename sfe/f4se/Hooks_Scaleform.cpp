#include "Hooks_Scaleform.h"
#include "f4se_common/Relocation.h"
#include "f4se_common/BranchTrampoline.h"
#include "xbyak/xbyak.h"

#include "f4se_common/f4se_version.h"
#include "common/IDirectoryIterator.h"
#include "f4se/PluginManager.h"


#include "ScaleformMovie.h"
#include "ScaleformValue.h"
#include "ScaleformCallbacks.h"
#include "ScaleformState.h"
#include "ScaleformLoader.h"
#include "ScaleformTranslator.h"


#include <unordered_set>

#include <xmmintrin.h>

#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio.hpp>
#include <deque>
#include <thread>
#include <mutex>
#include <chrono>
#include <ratio>
#include <ctime>
#include <shlobj.h>
#include <filesystem>  
#include <boost/algorithm/string.hpp>
#include <exception>

using namespace std;

ofstream logger;
GFxMovieView* mainview;
bool mainMenuOpen = false;
int chatOpenVirtualKey = 0xC0;
void testlogging(string testing)
{
	logger << testing << endl;
}


enum { max_length = 1024 };
using boost::asio::ip::tcp;

class SEExceptionHandler : public std::exception
{
private:
	unsigned int nSE;
public:
	SEExceptionHandler() : nSE{ 0 } {}
	SEExceptionHandler(unsigned int n) : nSE{ n } {}
	unsigned int getSeNumber() { return nSE; }
};
void trans_func(unsigned int u, EXCEPTION_POINTERS*)
{
	throw SEExceptionHandler(u);
}

class client
{
public:

	client(boost::asio::io_context& io_context,
		boost::asio::ssl::context& context,
		boost::asio::ip::tcp::resolver::iterator endpoint_iterator, GFxMovieRoot* movieRoot, GFxValue* instance_ptr)
		: socket_(io_context, context), io_context_(io_context), m_root(movieRoot), m_this(instance_ptr)
	{
		//socket_.set_verify_mode(boost::asio::ssl::verify_peer);
		SSL_set_cipher_list(socket_.native_handle(), SSL_DEFAULT_CIPHER_LIST);
		SSL_set_options(socket_.native_handle(), SSL_OP_NO_COMPRESSION);
		socket_.set_verify_mode(boost::asio::ssl::verify_none);
		socket_.set_verify_callback(
			boost::bind(&client::verify_certificate, this, _1, _2));

		boost::asio::async_connect(socket_.lowest_layer(), endpoint_iterator,
			boost::bind(&client::handle_connect, this,
				boost::asio::placeholders::error));

	}

	bool verify_certificate(bool preverified,
		boost::asio::ssl::verify_context& ctx)
	{

		//TODO: Check cert validity

		char subject_name[256];
		X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
		X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
		//logger << "Verifying " << subject_name << "\n";

		return true;

		return preverified;
	}

	void handle_connect(const boost::system::error_code& error)
	{
		if (!error)
		{
			socket_.async_handshake(boost::asio::ssl::stream_base::client,
				boost::bind(&client::handle_handshake, this,
					boost::asio::placeholders::error));
			//socket_.handshake(boost::asio::ssl::stream_base::client);
		}
		else
		{
			logger << "Connect failed: " << error.message() << "\n";
		}
	}

	void handle_handshake(const boost::system::error_code& error)
	{
		if (!error)
		{
			logger << "Handshake success" << endl;

			/*GFxValue dispatchEvent;
			GFxValue eventArgs[3];
			m_root->CreateString(&eventArgs[0], "connect");
			eventArgs[1] = GFxValue(true);
			eventArgs[2] = GFxValue(false);
			m_root->CreateObject(&dispatchEvent, "flash.events.Event", eventArgs, 3);
			m_this->SetMember("connected", &GFxValue(true));
			m_this.Invoke("dispatchEvent", nullptr, &dispatchEvent, 1);*/


			if (m_this->objectInterface == NULL)
			{
				mainMenuOpen = false;
				do_close();
				return;
			}
			else
			{
				GFxValue status;
				status.SetBool(true);

				m_this->SetMember("connected", &status);
			}



			boost::asio::steady_timer t(io_context_, boost::asio::chrono::milliseconds(50));
			t.async_wait(boost::bind(&client::constantly_read, this, boost::asio::placeholders::error));
		}
		else
		{
			logger << "Handshake failed: " << error.message() << "\n";
		}
	}
	void write(const string& msg)
	{
		io_context_.post(boost::bind(&client::do_write, this, msg));
	}

	void constantly_read(const boost::system::error_code& error)
	{
		//logger << "calling const_read" << endl;

		boost::asio::async_read(
			socket_, reply_, boost::asio::transfer_at_least(1),
			boost::bind(&client::handle_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

	}

	void do_write(string& msg)
	{
		//logger << "attempting to write the message2:" << msg.body() << endl;
		bool write_in_progress = !write_msgs_.empty();
		write_msgs_.push_back(msg);
		if (!write_in_progress)
		{
			boost::asio::async_write(socket_,
				boost::asio::buffer(write_msgs_.front(),
					write_msgs_.front().length()),
				boost::bind(&client::handle_write, this,
					boost::asio::placeholders::error));
		}
	}

	void handle_write(const boost::system::error_code& error)
	{
		if (!error)
		{
			write_msgs_.pop_front();
			if (!write_msgs_.empty())
			{
				boost::asio::async_write(socket_,
					boost::asio::buffer(write_msgs_.front(),
						write_msgs_.front().length()),
					boost::bind(&client::handle_write, this,
						boost::asio::placeholders::error));
			}
		}
		else
		{
			do_close();
		}
	}


	void do_close()
	{
		//socket_.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_type::shutdown_both);
		socket_.lowest_layer().close();
		//socket_.lowest_layer().release();
		//delete &socket_;
		logger << "Socket closing" << endl;
		try
		{
			if (m_this->objectInterface != NULL)
			{
				GFxValue status;
				status.SetBool(false);
				m_this->SetMember("connected", &status);
			}
		}
		//catch (...)
		catch(SEExceptionHandler& e)
		{
			//logger << "Access violation" << endl;
			if (e.getSeNumber() == EXCEPTION_ACCESS_VIOLATION)//This is recoverable, but SHOULDN'T be reachable
				logger << "Exception: Access Violation in do_close" << endl;
			else
			{
				throw;
			}
		}
		io_context_.stop();
	}


	void handle_read(boost::system::error_code ec, size_t bytes_received) {
		if (!ec)
		{
			boost::asio::streambuf::const_buffers_type bufs = reply_.data();
			std::string str(boost::asio::buffers_begin(bufs),
				boost::asio::buffers_begin(bufs) + reply_.size());
			//logger << "READING MESSAGE: bytes received: " << bytes_received << ", MESSAGE: " << str << endl;
			GFxValue obj;
			try
			{
				if (m_this->objectInterface == NULL)
				{
					mainMenuOpen = false;
					do_close();
					return;
				}

				//if (!m_root->GetVariable(&obj, "root.LoginComponent.instance18"))
				//	logger << "ERROR: could not get instance18";
				m_this->SetMember("bytesAvailable", &GFxValue((UInt32)(reply_.size())));
			}
			//catch (...)
			catch (SEExceptionHandler& e)
			{
				//logger << "Access violation" << endl;
				if (e.getSeNumber() == EXCEPTION_ACCESS_VIOLATION)//This is recoverable, but SHOULDN'T be reachable
				{
					logger << "Exception: Access Violation in handle_read" << endl;
					socket_.lowest_layer().close();
					io_context_.stop();
					return;
				}
				else
				{
					throw;
				}
			}

			/*GFxValue dispatchEvent;
			GFxValue eventArgs[3];
			m_root->CreateString(&eventArgs[0], "socketData");
			eventArgs[1] = GFxValue(true);
			eventArgs[2] = GFxValue(false);
			m_root->CreateObject(&dispatchEvent, "flash.events.ProgressEvent", eventArgs, 3);
			obj.Invoke("dispatchEvent", nullptr, &dispatchEvent, 1);*/

			boost::asio::async_read(
				socket_, reply_, boost::asio::transfer_at_least(1),
				boost::bind(&client::handle_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			logger << ec.message() << endl;
			do_close();
		}

	}

private:
	boost::asio::io_context& io_context_;
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
	deque<string> write_msgs_;
	GFxMovieRoot* m_root;
	GFxValue* m_this;
public:
	boost::asio::streambuf reply_;
};

class BSScaleformManager;

typedef BSScaleformManager * (*_BSScaleformManager_Ctor)(BSScaleformManager * mem);
RelocAddr <_BSScaleformManager_Ctor> BSScaleformManager_Ctor(0x02110310);
_BSScaleformManager_Ctor BSScaleformManager_Ctor_Original = nullptr;

typedef UInt32(*_BSScaleformTint)(BSGFxShaderFXTarget * value, float * colors, float multiplier);
RelocAddr <_BSScaleformTint> BSScaleformTint(0x020F2870);
_BSScaleformTint BSScaleformTint_Original = nullptr;

//RelocAddr <uintptr_t> ScaleformInitHook_Start(0x021109B0 + 0x188);
//RelocAddr <uintptr_t> ScaleformInitHook_Start(0x2792195);
//RelocAddr <uintptr_t> ScaleformInitHook_Start(0x2828105);
//RelocAddr <uintptr_t> ScaleformInitHook_Start(0x27BB2F5);
RelocAddr <uintptr_t> ScaleformInitHook_Start(0x2867B05);

RelocAddr <uintptr_t> IMenuCreateHook_Start(0x02042310 + 0x90A);

// D7C709A779249EBC0C50BB992E9FD088A33B282F+76
RelocAddr <uintptr_t> SetMenuName(0x01B41DB0);

//// plugin API
struct ScaleformPluginInfo
{
	const char	* name;
	F4SEScaleformInterface::RegisterCallback	callback;
};

typedef std::list <ScaleformPluginInfo> PluginList;
static PluginList s_plugins;

bool g_logScaleform = false;


bool RegisterScaleformPlugin(const char * name, F4SEScaleformInterface::RegisterCallback callback)
{
	// check for a name collision
	for (PluginList::iterator iter = s_plugins.begin(); iter != s_plugins.end(); ++iter)
	{
		if (!strcmp(iter->name, name))
		{
			_WARNING("scaleform plugin name collision: %s", iter->name);
			return false;
		}
	}

	ScaleformPluginInfo	info;

	info.name = name;
	info.callback = callback;

	s_plugins.push_back(info);

	return true;
}

void Hooks_Scaleform_Init()
{
	//
}

/*class F4SEScaleform_VisitMembers : public GFxValue::ObjectInterface::ObjVisitor
{
public:
	F4SEScaleform_VisitMembers(GFxMovieRoot * root, GFxValue * result) : m_root(root), m_result(result) { }
	virtual void Visit(const char * member, GFxValue * value) override
	{
		GFxValue str;
		m_root->CreateString(&str, member);
		m_result->PushBack(&str);
	}

	GFxMovieRoot * m_root;
	GFxValue * m_result;
};

class F4SEScaleform_GetMembers : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args* args)
	{
		if(args->numArgs >= 1)
		{
			args->movie->movieRoot->CreateArray(args->result);
			F4SEScaleform_VisitMembers dm(args->movie->movieRoot, args->result);
			args->args[0].VisitMembers(&dm);
		}
	}
};




class F4SEScaleform_GetDirectoryListing : public GFxFunctionHandler
{
public:
	virtual void Invoke(Args * args)
	{
		ASSERT(args->numArgs >= 1);
		ASSERT(args->args[0].GetType() == GFxValue::kType_String);

		const char * directory = args->args[0].GetString();
		const char * match = nullptr;
		if(args->numArgs >= 2)
			match = args->args[1].GetString();

		args->movie->movieRoot->CreateArray(args->result);

		for(IDirectoryIterator iter(directory, match); !iter.Done(); iter.Next())
		{
			std::string	path = iter.GetFullPath();
			WIN32_FIND_DATA * fileData = iter.Get();

			GFxValue fileInfo;
			args->movie->movieRoot->CreateObject(&fileInfo);

			GFxValue filePath;
			args->movie->movieRoot->CreateString(&filePath, path.c_str());
			fileInfo.SetMember("nativePath", &filePath);

			GFxValue fileName;
			args->movie->movieRoot->CreateString(&fileName, fileData->cFileName);
			fileInfo.SetMember("name", &filePath);

			SYSTEMTIME sysTime;
			FileTimeToSystemTime(&fileData->ftLastWriteTime, &sysTime);

			GFxValue date;
			GFxValue params[7];
			params[0].SetNumber(sysTime.wYear);
			params[1].SetNumber(sysTime.wMonth - 1); // Flash Month is 0-11, System time is 1-12
			params[2].SetNumber(sysTime.wDay);
			params[3].SetNumber(sysTime.wHour);
			params[4].SetNumber(sysTime.wMinute);
			params[5].SetNumber(sysTime.wSecond);
			params[6].SetNumber(sysTime.wMilliseconds);
			args->movie->movieRoot->CreateObject(&date, "Date", params, 7);
			fileInfo.SetMember("lastModified", &date);

			FileTimeToSystemTime(&fileData->ftCreationTime, &sysTime);
			params[0].SetNumber(sysTime.wYear);
			params[1].SetNumber(sysTime.wMonth - 1); // Flash Month is 0-11, System time is 1-12
			params[2].SetNumber(sysTime.wDay);
			params[3].SetNumber(sysTime.wHour);
			params[4].SetNumber(sysTime.wMinute);
			params[5].SetNumber(sysTime.wSecond);
			params[6].SetNumber(sysTime.wMilliseconds);
			args->movie->movieRoot->CreateObject(&date, "Date", params, 7);
			fileInfo.SetMember("creationDate", &date);

			fileInfo.SetMember("isDirectory", &GFxValue((fileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY));
			fileInfo.SetMember("isHidden", &GFxValue((fileData->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == FILE_ATTRIBUTE_HIDDEN));
			args->result->PushBack(&fileInfo);
		}
	}
};*/


string GetActiveWindowTitle()
{
	char wnd_title[256];
	HWND hwnd = GetForegroundWindow(); // get handle of currently active window
	GetWindowText(hwnd, wnd_title, sizeof(wnd_title));
	return wnd_title;
}

void checkKeyState(GFxValue* thisptr, GFxMovieRoot* root)
{
	bool keyDown = false;
	bool escKeyDown = false;

	_set_se_translator(trans_func);

	chrono::high_resolution_clock::time_point t1;
	chrono::high_resolution_clock::time_point t2;


	while (true && mainMenuOpen)
	{

		if (thisptr->objectInterface == NULL)
		{
			logger << "Interface is null. Shutting down listener." << endl;
			mainMenuOpen = false;
			return;
		}
		if (GetActiveWindowTitle() == "Fallout76")
		{
			//if (GetKeyState(0xC0) < 0)
			//int key = (byte)(VkKeyScanExA('`', GetKeyboardLayout(0)));
			//if (GetKeyState(key) < 0)
			//int key = MapVirtualKeyExA(0x29, MAPVK_VSC_TO_VK, GetKeyboardLayout(0));
			//logger << "keyresult: " << key << endl;
			//logger << chatOpenVirtualKey << endl;
			if (GetKeyState(chatOpenVirtualKey) < 0)
			{
				if (!keyDown)
				{
					keyDown = true;
					t1 = chrono::high_resolution_clock::now();
				}
			}
			else if (GetKeyState(VK_ESCAPE) < 0)
			{
				if (!escKeyDown)
				{
					escKeyDown = true;
				}
			}
			else if (keyDown || escKeyDown)
			{
				try
				{
					if (keyDown)
					{
						t2 = chrono::high_resolution_clock::now();
						chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);

						if (thisptr->objectInterface != NULL)
						{
							GFxValue as3Root, emptyVal, chatOpen;
							root->GetVariable(&as3Root, "root1");
							if (as3Root.HasMember("chatOpenPressed") && as3Root.GetMember("chatOpenPressed", &chatOpen))
							{
								if (!chatOpen.GetBool())
								{
									GFxValue result;
									//as3Root.Invoke("enterChatMode", &result, &emptyVal, 0);//causes crash
									as3Root.SetMember("chatOpenPressed", &GFxValue(true));
									//logger << "calling invoke" << endl;
								}
							}
						}
						else
						{
							return;
						}

						keyDown = false;
					}
					else if (escKeyDown)
					{
						if (thisptr->objectInterface != NULL)
						{
							GFxValue as3Root, chatOpen;
							root->GetVariable(&as3Root, "root1");
							if (as3Root.HasMember("chatOpenPressed") && as3Root.GetMember("chatOpenPressed", &chatOpen))
							{
								if (chatOpen.GetBool())
								{
									GFxValue emergencyPressed;
									if (as3Root.HasMember("emergencyClosePressed") && as3Root.GetMember("emergencyClosePressed", &emergencyPressed))
									{
										if (!emergencyPressed.GetBool())
											as3Root.SetMember("emergencyClosePressed", &GFxValue(true));
									}
								}
							}
						}
						else
						{
							return;
						}

						escKeyDown = false;
					}

				}
				catch (SEExceptionHandler& e)
				{
					//logger << "Access violation" << endl;
					if (e.getSeNumber() == EXCEPTION_ACCESS_VIOLATION)//This is recoverable, but SHOULDN'T be reachable
					{
						logger << "Exception: Access Violation in checkKeyState" << endl;
						return;
					}
					else
					{
						throw;
					}
				}
			}
		}
		Sleep(5);
	}
}

GFxValue*base;
boost::asio::ssl::context* ctx_perm = NULL;
std::thread* t_perm = NULL;
class ScaleformExtender_Connect : public GFxFunctionHandler
{
public:
	GFxValue* thisptr = NULL;
	std::unique_ptr<client> cli;
	virtual void	Invoke(Args* args)
	{
		if (args->numArgs >= 1)
		{
			std::string s = args->args[0].GetString();

			if (s == "register")
			{
				/*GFxValue dispatchEvent;
				GFxValue eventArgs[3];
				args->movie->movieRoot->CreateString(&eventArgs[0], "socketData");
				eventArgs[1] = GFxValue(true);
				eventArgs[2] = GFxValue(false);
				args->args[1].Invoke("dispatchEvent", nullptr, eventArgs, 3);*/

				GFxValue* temp = new GFxValue(&args->args[1]);
				thisptr = temp;
				temp->AddManaged();
				mainMenuOpen = true;
				std::thread* keyCheck = new std::thread(checkKeyState, temp, args->movie->movieRoot);
			}
			if (s == "connect")
			{
				//io_context_perm = *io_context;
				boost::asio::io_context*  io_context_ptr = new boost::asio::io_context;
				//boost::asio::io_context io_context = *io_context_ptr;
				boost::asio::ip::tcp::resolver resolver(*io_context_ptr);
				boost::asio::ip::tcp::resolver::query query(args->args[1].GetString(), args->args[2].GetString());
				boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

				//boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
				boost::asio::ssl::context* ctx = new boost::asio::ssl::context(boost::asio::ssl::context::sslv23);

				ctx_perm = ctx;

				//client *c = new client(*io_context_ptr, *ctx, iterator, args->movie->movieRoot, temp);
				cli = make_unique<client>(*io_context_ptr, *ctx, iterator, args->movie->movieRoot, thisptr);


				//std::thread* t = new std::thread([this]() { io_context.run(); });
				thread* t = new thread([io_context_ptr]() { _set_se_translator(trans_func); io_context_ptr->run(); });
				t_perm = t;
				//t->detach();
				//t->join();
			}
			if (s == "writeUTFBytes")
			{
				//logger << "attempting to write the message" << args->args[1].GetString() << endl;
				string msg(args->args[1].GetString());
				if (cli != nullptr)
				{
					cli->write(msg);
					args->result->SetBool(true);
				}
				else
				{
					logger << "Client is null. Returning write failure." << endl;
					args->result->SetBool(false);
				}
				/*GFxValue dispatchEvent;
				GFxValue eventArgs[3];
				args->movie->movieRoot->CreateString(&eventArgs[0], "socketData");
				eventArgs[1] = GFxValue(true);
				eventArgs[2] = GFxValue(false);
				args->movie->movieRoot->CreateObject(&dispatchEvent, "flash.events.ProgressEvent", eventArgs, 3);
				args->args[2].Invoke("dispatchEvent", nullptr, &dispatchEvent, 1);*/
			}
			if (s == "readByte")
			{
				if (cli == nullptr)
				{
					args->result->SetInt((SInt32)0);
					logger << "Client is null. Returning null value." << endl;//Properly handle errors in the future
				}
				else
				{
					std::string line(
						boost::asio::buffers_begin(cli->reply_.data()),
						boost::asio::buffers_begin(cli->reply_.data()) + 1);
					cli->reply_.consume(1);
					GFxValue socketObj, old;
					//socketObj.GetMember("bytesAvailable", &old);
					//socketObj.SetMember("bytesAvailable", &GFxValue((UInt32)(old.GetInt() - 1)));
					thisptr->GetMember("bytesAvailable", &old);
					thisptr->SetMember("bytesAvailable", &GFxValue((UInt32)(old.GetUInt() - 1)));
					char ch = line[0];
					//logger << (SInt32)ch << endl;
					args->result->SetInt((SInt32)ch);
				}

			}
			if (s == "readUTFBytes")
			{
				if (cli == nullptr)
				{
					args->result->SetString("$$IOERROR$$");
					logger << "Client is null. Sending IOError to client." << endl;
				}
				else
				{
					UInt32 num = args->args[1].GetUInt();
					std::string line(
						boost::asio::buffers_begin(cli->reply_.data()),
						boost::asio::buffers_begin(cli->reply_.data()) + num);
					cli->reply_.consume(num);
					//logger << "Read request: " << line << endl;
					GFxValue socketObj, old;
					thisptr->GetMember("bytesAvailable", &old);
					thisptr->SetMember("bytesAvailable", &GFxValue(old.GetUInt() - num));
					args->result->SetString(line.c_str());
				}
			}

			if (s == "writePerksFile")
			{
				char curPath[MAX_PATH];
				if (GetCurrentDirectory(MAX_PATH, curPath)) 
				{
					string dest = curPath;
					//logger << "Current directory found: " << dest << endl;
					dest += "\\Data\\perkloadoutmanager.ini";
	
					ofstream save;
					save.open(dest, ofstream::out | ofstream::trunc);
					if (save.is_open())
					{
						save << args->args[1].GetString();//data
						args->result->SetBool(true);
						save.close();
					}
					else
						args->result->SetBool(false);
				}
				else
					logger << "Couldn't retrieve the current directory.";
			}
			if (s == "writeSaveEverythingFile")
			{
				char curPath[MAX_PATH];
				if (GetCurrentDirectory(MAX_PATH, curPath)) 
				{
					string dest = curPath;
					dest += "\\Data\\saveeverything.ini";
					ofstream save;
					save.open(dest, ofstream::out | ofstream::trunc);
					if (save.is_open())
					{
						save << args->args[1].GetString();//data
						args->result->SetBool(true);
						save.close();
					}
					else
						args->result->SetBool(false);
				}
				else
					logger << "Couldn't retrieve the current directory.";
			}
			if (s == "updateChatHotkey")
			{
				char hotkey = NULL;
				string hotkeyStr = args->args[1].GetString();
				boost::trim(hotkeyStr);
				if (hotkeyStr.length() == 1)
					hotkey = hotkeyStr[0];
				else if (hotkeyStr.find("DELETE") != string::npos)
					chatOpenVirtualKey = VK_DELETE;
				else if (hotkeyStr.find("INSERT") != string::npos)
					chatOpenVirtualKey = VK_INSERT;
				else if (hotkeyStr.find("HOME") != string::npos)
					chatOpenVirtualKey = VK_HOME;
				else if (hotkeyStr.find("END") != string::npos)
					chatOpenVirtualKey = VK_END;
				//logger << "hotkeystr:" << hotkeyStr <<  ", hotkeystr length: " << hotkeyStr.length() << endl;

				if (hotkey != NULL)
				{
					char result = (char)(VkKeyScanExA(hotkey, GetKeyboardLayout(0)));
					//logger << (int)result << endl;
					if (result != -1)
						chatOpenVirtualKey = (unsigned char)result;
				}
			}
		}
	}
};

/*typedef struct {
	__m128 array;
} hva;    // 2 element HVA type on __m128*/


void dumproot(GFxValue* ele, string format = "", int depth = 0)
{
	++depth;
	GFxValue child, numChildren, name;
	if (depth >= 50)
	{
		testlogging("recursiondepth >= 50, bailing");
		return;
	}

	if (ele->GetMember("numChildren", &numChildren))
	{
		for (SInt32 i = 0; i < numChildren.GetInt(); ++i)
		{
			GFxValue index(i);
			ele->Invoke("getChildAt", &child, &index, 1);
			if (child.HasMember("name") && child.GetMember("name", &name))
			{
				GFxValue stringname, value;
				ele->Invoke("toString", &stringname, nullptr, 0);
				testlogging(format + "[" + to_string(i) + "]\t " + name.GetString() + " | " + stringname.GetString());
				if (child.HasMember("onF4SEObjCreated"))
					testlogging(format + "Element has objcreated function");
			}
			dumproot(&child, "\t" + format, depth);

		}
	}
}
void ScaleformInitHook_Install(GFxMovieView * view)
{
	hva test;
	GFxValue root;
	GFxMovieRoot	* movieRoot = view->movieRoot;


	bool result = movieRoot->GetVariable(&root, "root1");


	if (!result)
	{
		testlogging("couldn't get root");
		return;
	}

	//dumproot(&root);


	GFxValue	f4se;
	movieRoot->CreateObject(&f4se);
	base = &f4se;
	GFxValue	version;
	//movieRoot->CreateObject(&version);

	//f4se.SetMember("version", &version);

	//version.SetMember("major", &GFxValue(0));


	/*GFxValue funcObj;

	if (root.HasMember("onF4SEObjCreated") && root.GetMember("onF4SEObjCreated", &funcObj) && funcObj.IsFunction())
	{
		root.Invoke("onF4SEObjCreated", nullptr, &f4se, 1);
	}

	GFxValue name2;
	if (root.HasMember("name") && root.GetMember("name", &name2))
	{
		testlogging(name2.GetString());
	}

	GFxValue child;
	GFxValue numChildren;

	if (root.GetMember("numChildren", &numChildren))
	{
		testlogging("child found");
		for (SInt32 i = 0; i < numChildren.GetInt(); ++i)
		{
			GFxValue index(i);
			root.Invoke("getChildAt", &child, &index, 1);
			GFxValue name;


			GFxValue child2;
			GFxValue numChildren2;


			if (child.HasMember("onF4SEObjCreated") && child.GetMember("onF4SEObjCreated", &funcObj) && funcObj.IsFunction())
			{
				RegisterFunction<ScaleformExtender_Connect>(&f4se, movieRoot, "call");
				child.Invoke("onF4SEObjCreated", nullptr, &f4se, 1);
				testlogging("Called objcreated.2");
			}
		}
	}*/

	GFxValue codeObj;

	if (root.HasMember("__SFCodeObj") && root.GetMember("__SFCodeObj", &codeObj) && codeObj.IsObject())
	{
		testlogging("Set code object, 1st call");
		RegisterFunction<ScaleformExtender_Connect>(&f4se, movieRoot, "call");
		root.SetMember("__SFCodeObj", &f4se);
		return;
	}

	GFxValue name2;
	/*if (root.HasMember("name") && root.GetMember("name", &name2))
	{
		testlogging(name2.GetString());
	}*/

	GFxValue child;
	GFxValue numChildren;

	if (root.GetMember("numChildren", &numChildren))
	{
		//testlogging("child found");
		for (SInt32 i = 0; i < numChildren.GetInt(); ++i)
		{
			GFxValue index(i);
			root.Invoke("getChildAt", &child, &index, 1);
			GFxValue name;

			/*if (child.HasMember("name") && child.GetMember("name", &name))
			{
				testlogging(name.GetString());
			}*/

			GFxValue child2;
			GFxValue numChildren2;


			if (child.HasMember("__SFCodeObj") && child.GetMember("__SFCodeObj", &codeObj) && codeObj.IsObject())
			{
				RegisterFunction<ScaleformExtender_Connect>(&f4se, movieRoot, "call");
				child.SetMember("__SFCodeObj", &f4se);
				testlogging("Set code object, 2nd call");
			}
		}
	}


	return;

	/*RegisterFunction<F4SEScaleform_GetMembers>(&f4se, movieRoot, "GetMembers");
	RegisterFunction<F4SEScaleform_AllowTextInput>(&f4se, movieRoot, "AllowTextInput");
	RegisterFunction<F4SEScaleform_SendExternalEvent>(&f4se, movieRoot, "SendExternalEvent");
	RegisterFunction<F4SEScaleform_CallFunctionNoWait>(&f4se, movieRoot, "CallFunctionNoWait");
	RegisterFunction<F4SEScaleform_CallGlobalFunctionNoWait>(&f4se, movieRoot, "CallGlobalFunctionNoWait");
	RegisterFunction<F4SEScaleform_GetDirectoryListing>(&f4se, movieRoot, "GetDirectoryListing");
	RegisterFunction<F4SEScaleform_MountImage>(&f4se, movieRoot, "MountImage");
	RegisterFunction<F4SEScaleform_UnmountImage>(&f4se, movieRoot, "UnmountImage");

	GFxValue	version;
	movieRoot->CreateObject(&version);


	version.SetMember("major", &GFxValue(F4SE_VERSION_INTEGER));
	version.SetMember("minor", &GFxValue(F4SE_VERSION_INTEGER_MINOR));
	version.SetMember("beta", &GFxValue(F4SE_VERSION_INTEGER_BETA));
	version.SetMember("releaseIdx", &GFxValue(F4SE_VERSION_RELEASEIDX));
	f4se.SetMember("version", &version);
	// plugins
	GFxValue	plugins;
	movieRoot->CreateObject(&plugins);

	for(PluginList::iterator iter = s_plugins.begin(); iter != s_plugins.end(); ++iter)
	{
		GFxValue	plugin;
		movieRoot->CreateObject(&plugin);

		iter->callback(view, &plugin);

		plugins.SetMember(iter->name, &plugin);
	}

	f4se.SetMember("plugins", &plugins);

	root.SetMember("f4se", &f4se);

	if(root.IsObject())
	{
		GFxValue funcObj;
		if(root.HasMember("onF4SEObjCreated") && root.GetMember("onF4SEObjCreated", &funcObj) && funcObj.IsFunction())
		{
			root.Invoke("onF4SEObjCreated", nullptr, &f4se, 1);
		}

		GFxValue child;
		GFxValue numChildren;
		if(root.GetMember("numChildren", &numChildren))
		{
			for(SInt32 i = 0; i < numChildren.GetInt(); ++i)
			{
				GFxValue index(i);
				root.Invoke("getChildAt", &child, &index, 1);

				if(child.HasMember("onF4SEObjCreated") && child.GetMember("onF4SEObjCreated", &funcObj) && funcObj.IsFunction())
				{
					child.Invoke("onF4SEObjCreated", nullptr, &f4se, 1);
				}
			}
		}
	}

	GFxValue dispatchEvent;
	GFxValue eventArgs[3];
	movieRoot->CreateString(&eventArgs[0], "F4SE::Initialized");
	eventArgs[1] = GFxValue(true);
	eventArgs[2] = GFxValue(false);
	movieRoot->CreateObject(&dispatchEvent, "flash.events.Event", eventArgs, 3);
	movieRoot->Invoke("root.dispatchEvent", nullptr, &dispatchEvent, 1);*/
}


BSScaleformManager * BSScaleformManager_Ctor_Hook(BSScaleformManager * mgr)
{
	BSScaleformManager * result = BSScaleformManager_Ctor_Original(mgr);

	BSScaleformTranslator * translator = (BSScaleformTranslator*)result->stateBag->GetStateAddRef(GFxState::kInterface_Translator);


	if (g_logScaleform)
	{
		GFxLogState * logger = (GFxLogState*)result->stateBag->GetStateAddRef(GFxState::kInterface_Log);
		logger->logger = new F4SEGFxLogger();
	}


	return result;
}

UInt32 BSScaleformTint_Hook(BSGFxShaderFXTarget * value, float * colors, float multiplier)
{
	if (value->HasMember("onApplyColorChange"))
	{
		GFxValue result;
		GFxValue args[4];
		args[0] = GFxValue(colors[0]);
		args[1] = GFxValue(colors[1]);
		args[2] = GFxValue(colors[2]);
		args[3] = GFxValue(multiplier);
		value->Invoke("onApplyColorChange", &result, args, 4);
		if (result.IsArray() && result.GetArraySize() >= 4)
		{
			result.GetElement(0, &args[0]);
			result.GetElement(1, &args[1]);
			result.GetElement(2, &args[2]);
			result.GetElement(3, &args[3]);

			colors[0] = args[0].GetNumber();
			colors[1] = args[1].GetNumber();
			colors[2] = args[2].GetNumber();
			multiplier = args[3].GetNumber();
		}
	}

	return BSScaleformTint_Original(value, colors, multiplier);
}

/*void Hooks_Scaleform_Commit()
{
	UInt32	logScaleform = 0;
	if(GetConfigOption_UInt32("Interface", "bEnableGFXLog", &logScaleform))
	{
		if(logScaleform)
		{
			g_logScaleform = true;
		}
	}

	// hook creation of each menu
	{
		struct ScaleformInitHook_Code : Xbyak::CodeGenerator {
			ScaleformInitHook_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
			{
				mov(eax, 1);
				lock(); xadd(ptr [rcx + 8], eax);
				jmp((void *)&ScaleformInitHook_Install);
			}
		};

		void * codeBuf = g_localTrampoline.StartAlloc();
		ScaleformInitHook_Code code(codeBuf);
		g_localTrampoline.EndAlloc(code.getCurr());

		g_branchTrampoline.Write5Call(ScaleformInitHook_Start, uintptr_t(code.getCode()));
	}


	// hook creation of BSScaleformManager
	{
		struct BSScaleformManager_Code : Xbyak::CodeGenerator {
			BSScaleformManager_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label retnLabel;

				push(rbp);
				push(rbx);
				push(rsi);
				push(rdi);

				jmp(ptr [rip + retnLabel]);

				L(retnLabel);
				dq(BSScaleformManager_Ctor.GetUIntPtr() + 5);
			}
		};

		void * codeBuf = g_localTrampoline.StartAlloc();
		BSScaleformManager_Code code(codeBuf);
		g_localTrampoline.EndAlloc(code.getCurr());

		BSScaleformManager_Ctor_Original = (_BSScaleformManager_Ctor)codeBuf;

		g_branchTrampoline.Write5Branch(BSScaleformManager_Ctor.GetUIntPtr(), (uintptr_t)BSScaleformManager_Ctor_Hook);
	}

	// hook global tinting of objects
	{
		struct BSScaleformTint_Code : Xbyak::CodeGenerator {
			BSScaleformTint_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label retnLabel;

				mov(ptr[rsp+0x18], rbx);

				jmp(ptr [rip + retnLabel]);

				L(retnLabel);
				dq(BSScaleformTint.GetUIntPtr() + 5);
			}
		};

		void * codeBuf = g_localTrampoline.StartAlloc();
		BSScaleformTint_Code code(codeBuf);
		g_localTrampoline.EndAlloc(code.getCurr());

		BSScaleformTint_Original = (_BSScaleformTint)codeBuf;

		g_branchTrampoline.Write5Branch(BSScaleformTint.GetUIntPtr(), (uintptr_t)BSScaleformTint_Hook);
	}

	// Hook menu construction
	{
		struct MenuConstruction_Code : Xbyak::CodeGenerator {
			MenuConstruction_Code(void * buf, uintptr_t originFuncAddr, uintptr_t funcAddr) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label retnLabel, funcLabel1, funcLabel2;

				// Put the original call back
				call(ptr [rip + funcLabel1]);

				// Pull the IMenu off the stack and call our new function
				mov(rcx, ptr[rsp+0x388-0x348]);
				call(ptr [rip + funcLabel2]);

				// Jump back to the original location
				jmp(ptr [rip + retnLabel]);

				L(funcLabel1);
				dq(originFuncAddr);

				L(funcLabel2);
				dq(funcAddr);

				L(retnLabel);
				dq(IMenuCreateHook_Start.GetUIntPtr() + 0x5);
			}
		};

		void * codeBuf = g_localTrampoline.StartAlloc();
		MenuConstruction_Code code(codeBuf, SetMenuName.GetUIntPtr(), (uintptr_t)LoadCustomMenu_Hook);
		g_localTrampoline.EndAlloc(code.getCurr());
		g_branchTrampoline.Write5Branch(IMenuCreateHook_Start.GetUIntPtr(), uintptr_t(code.getCode()));
	}
}*/

void Hooks_Scaleform_Commit()
{
	char logPath[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, NULL, logPath))) {
		strcat_s(logPath, "\\My Games\\Fallout 76\\SFE\\SFEDebug.log");
		logger.open(logPath);
	}
	/*UInt32	logScaleform = 0;
	if (GetConfigOption_UInt32("Interface", "bEnableGFXLog", &logScaleform))
	{
		if (logScaleform)
		{
			g_logScaleform = true;
		}
	}*/

	// hook creation of each menu
	{
		struct ScaleformInitHook_Code : Xbyak::CodeGenerator {
			ScaleformInitHook_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
			{
				mov(edx, 1);
				lock(); xadd(ptr[rcx + 8], edx);
				mov(eax, edx);
				jmp((void *)&ScaleformInitHook_Install);
			}
		};

		void * codeBuf = g_localTrampoline.StartAlloc();
		ScaleformInitHook_Code code(codeBuf);
		g_localTrampoline.EndAlloc(code.getCurr());

		g_branchTrampoline.Write5Call(ScaleformInitHook_Start, uintptr_t(code.getCode()));
	}
}