#include <mex.h>

#include <Windows.h>

#include <deque>
#include <string>

static FINDEX_SEARCH_OPS search_ops(FindExSearchNameMatch);
static DWORD flags(0);
static std::wstring hint;
static int maxhits(0);
static std::deque<mxArray *> hits;

void file_search(const std::wstring &path)
{
  auto utf16ToMxArray = [](const std::wstring &in) -> mxArray * {
    size_t len = in.size();
    mwSize dims[2]{1, len};
    mxArray *out = mxCreateCharArray(2, dims);
    std::copy_n(in.c_str(), len, (wchar_t *)mxGetData(out));
    return out;
  };

  std::wstring hint_path = path + hint;

  WIN32_FIND_DATAW FindFileData;
  HANDLE hFind = FindFirstFileExW(hint_path.c_str(), FindExInfoBasic, &FindFileData, search_ops, NULL, flags);
  if (hFind != INVALID_HANDLE_VALUE)
  {
    do
    {
      hits.push_back(utf16ToMxArray(path + FindFileData.cFileName));
      if (maxhits > 0 && hits.size() > maxhits)
        break;
    } while (FindNextFileW(hFind, &FindFileData));
  }
  FindClose(hFind);
}

void traverse_dirs(std::wstring directory)
{
  // run the file search on the current directory
  file_search(directory);
  if (maxhits > 0 && hits.size() > maxhits)
    return;

  // recurse on its subdirectories
  std::wstring tmp = directory + L"*";
  WIN32_FIND_DATAW file;
  HANDLE search_handle = FindFirstFileW(tmp.c_str(), &file);
  if (search_handle != INVALID_HANDLE_VALUE)
  {
    do
    {
      if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
        if ((!lstrcmpW(file.cFileName, L".")) || (!lstrcmpW(file.cFileName, L"..")))
          continue;
      }

      tmp = directory + std::wstring((wchar_t *)file.cFileName);

      if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        traverse_dirs(tmp + L"\\");

    } while (FindNextFileW(search_handle, &file));
    FindClose(search_handle);
  }
}

// filepaths = findfile('hint',K,...)
// Options:
// - 'dironly'
// - 'recursive'
// - 'basedir' 'path'
// - 'casesensitive'
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  if (!nrhs)
    mexErrMsgIdAndTxt("findfile:invalidArguments", "Input argument cannot be empty");

  if (!mxIsChar(prhs[0]))
    mexErrMsgIdAndTxt("findfile:invalidArguments", "First input argument must be a string");
  size_t hint_len = mxGetNumberOfElements(prhs[0]);
  wchar_t *hint_src = (wchar_t *)mxGetChars(prhs[0]);

  if (nrhs >1 )
  {
    if (!(mxIsNumeric(prhs[1]) && mxGetNumberOfElements(prhs[1])<=1))
      mexErrMsgIdAndTxt("findfile:invalidArguments", "Second input argument must be a finite integer scalar.");
    if (mxIsEmpty(prhs[1]))
      maxhits = 0;
    else
    {
      double val = mxGetScalar(prhs[1]);
      if (val<0 || !mxIsFinite(val) || val!=(double)(int)val)
        mexErrMsgIdAndTxt("findfile:invalidArguments", "Second input argument must be a finite integer scalar.");
      maxhits = (int)val;
    }
  }
  else
    maxhits = 0;

  std::wstring basedir;
  if (nrhs > 2)
  {
    if (!mxIsChar(prhs[2]))
      mexErrMsgIdAndTxt("findfile:invalidArguments", "Third input argument must be a string");
    basedir.assign((wchar_t *)mxGetChars(prhs[2]));
  }

  hint.assign(hint_src, hint_src + hint_len); // global

  // traverse subdirectories
  if ((hint[1] == ':') || (hint[0] == '.') || (hint[0] == '\\')) // full path given
  {
    if (basedir.size())
      mexErrMsgIdAndTxt("findfile:mixedCommands", "Cannot specify base directory to search with a full path.");
    file_search(hint);
  }
  else
  {
    if (basedir.empty())
      basedir.assign(L".\\");
    else if (basedir.back() != '\\')
      basedir.append(L"\\");
    traverse_dirs(basedir);
  }

  plhs[0] = mxCreateCellMatrix(hits.size(), 1);
  for (int i = 0; i < hits.size(); ++i)
  {
    mxSetCell(plhs[0], i, hits.front());
    hits.pop_front();
  }
}
