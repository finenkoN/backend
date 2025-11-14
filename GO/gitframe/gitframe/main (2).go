//go:build !solution

package main

import (
	"bytes"
	"encoding/csv"
	"encoding/json"
	"errors"
	"fmt"
	"os"
	"os/exec"
	"path"
	"sort"
	"strings"
	"text/tabwriter"

	"github.com/finenko/gitfame/configs/configs"

	flag "github.com/spf13/pflag"
)

type SubjectInfo struct {
	Lines   int
	Commits map[string]struct{}
	Files   map[string]struct{}
}

type SubjectsMap map[string]*SubjectInfo

type SortOrder string

const (
	SortOrderLines   SortOrder = "lines"
	SortOrderCommits SortOrder = "commits"
	SortOrderFiles   SortOrder = "files"
)

func (so *SortOrder) Set(val string) error {
	switch val {
	case string(SortOrderLines):
		*so = SortOrderLines
	case string(SortOrderCommits):
		*so = SortOrderCommits
	case string(SortOrderFiles):
		*so = SortOrderFiles
	default:
		return errors.New("wrong sort order")
	}
	return nil
}

func (so *SortOrder) String() string {
	if so == nil {
		return ""
	}
	return fmt.Sprint(*so)
}

func (so *SortOrder) Type() string {
	return "SortOrder"
}

type Format string

const (
	FormatTabular   Format = "tabular"
	FormatCSV       Format = "csv"
	FormatJSON      Format = "json"
	FormatJSONLines Format = "json-lines"
)

func (f *Format) Set(val string) error {
	switch val {
	case string(FormatTabular):
		*f = FormatTabular
	case string(FormatCSV):
		*f = FormatCSV
	case string(FormatJSON):
		*f = FormatJSON
	case string(FormatJSONLines):
		*f = FormatJSONLines
	default:
		return errors.New("wrong output format")
	}

	return nil
}

func (f *Format) String() string {
	if f == nil {
		return ""
	}
	return fmt.Sprint(*f)
}

func (f *Format) Type() string {
	return "Format"
}

type ListStringFlag []string

func (ls *ListStringFlag) Set(val string) error {
	*ls = append(*ls, strings.Split(val, ",")...)
	return nil
}

func (ls *ListStringFlag) String() string {
	if ls == nil {
		return ""
	}
	return strings.Join(*ls, ",")
}

func (ls *ListStringFlag) Type() string {
	return "listString"
}

type languageExtension struct {
	extensions map[string]struct{}
	languages  map[string]struct{}
}

func parseExtension() (*languageExtension, error) {
	type Language struct {
		Name       string   `json:"name"`
		Type       string   `json:"type"`
		Extensions []string `json:"extensions"`
	}

	var languageExtensionJSON []Language
	err := json.Unmarshal(configs.JSONData, &languageExtensionJSON)
	if err != nil {
		return nil, err
	}
	langExt := &languageExtension{
		extensions: make(map[string]struct{}),
		languages:  make(map[string]struct{}),
	}

	filteredLanguages := make(map[string]struct{})
	for _, l := range languagesFlag {
		filteredLanguages[l] = struct{}{}
	}

	for _, l := range languageExtensionJSON {
		if _, ok := filteredLanguages[strings.ToLower(l.Name)]; ok {
			for _, e := range l.Extensions {
				langExt.languages[e] = struct{}{}
			}
		}
	}

	for _, e := range extensionsFlag {
		langExt.extensions[e] = struct{}{}
	}
	return langExt, nil
}

func ApproveFilename(filename string) bool {
	extentionsInfo, _ := parseExtension()

	_, isExtensionApproved := extentionsInfo.extensions[path.Ext(filename)]
	_, isLanguageApproved := extentionsInfo.languages[path.Ext(filename)]

	if !((len(extensionsFlag) == 0 || isExtensionApproved) && (len(languagesFlag) == 0 || isLanguageApproved)) {
		return false
	}

	for _, blobPattern := range excludeFlag {
		if isMatch, _ := path.Match(blobPattern, filename); isMatch {
			return false
		}
	}

	satisfyRestriction := true
	for _, blobPattern := range restrictToFlag {
		satisfyRestriction = false

		if isMatch, _ := path.Match(blobPattern, filename); isMatch {
			satisfyRestriction = true
			break
		}
	}

	return satisfyRestriction
}

type CommitInfo struct {
	author    string
	commit    string
	lineCount int
}

func emptyFileProcessor(name string) (*CommitInfo, error) {
	format := "%H\n%an\n%cn" // hash, author, commiter
	cmd := exec.Command("git", "-C", repositoryFlag, "log", revisionFlag, "-n", "1", "--pretty=format:"+format, "--", name)
	var out bytes.Buffer
	cmd.Stdout = &out
	err := cmd.Run()
	if err != nil {
		return nil, err
	}

	parts := strings.Split(out.String(), "\n")
	if len(parts) < 3 {
		return nil, errors.New("unexpected git log format")
	}

	author := parts[1]
	if useCommitterFlag {
		author = parts[2]
	}

	return &CommitInfo{
		commit:    parts[0],
		author:    author,
		lineCount: 0,
	}, nil
}

func fileProcessor(filePath string) (map[string]*CommitInfo, error) {
	cmd := exec.Command("git", "-C", repositoryFlag, "blame", filePath, "--porcelain", revisionFlag)
	var out bytes.Buffer
	cmd.Stdout = &out
	err := cmd.Run()
	if err != nil {
		return nil, err
	}

	commits := make(map[string]*CommitInfo)

	lines := strings.Split(out.String(), "\n")
	lines = lines[:len(lines)-1]

	if len(lines) == 0 {
		commit, err := emptyFileProcessor(filePath)
		if err != nil {
			return nil, err
		}

		commits[commit.commit] = commit
		return commits, err
	}

	for i := 0; i < len(lines); {
		hash := lines[i][:40]

		if commitInfo, ok := commits[hash]; ok {
			commitInfo.lineCount++
			i += 2
		} else {
			author := strings.TrimPrefix(lines[i+1], "author ")
			if useCommitterFlag {
				author = strings.TrimPrefix(lines[i+5], "committer ")
			}

			commits[hash] = &CommitInfo{
				commit:    hash,
				author:    author,
				lineCount: 1,
			}
			if !strings.HasPrefix(lines[i+10], "filename") {
				i++
			}
			i += 12
		}
	}

	return commits, nil
}

func getSubjectsInfo() (SubjectsMap, error) {
	subjectsList := SubjectsMap(make(map[string]*SubjectInfo))

	cmd := exec.Command("git", "-C", repositoryFlag, "ls-tree", "-r", "--name-only", revisionFlag)
	var out bytes.Buffer
	cmd.Stdout = &out
	err := cmd.Run()
	if err != nil {
		return nil, err
	}

	for _, filePath := range strings.Split(out.String(), "\n") {
		isApproved := ApproveFilename(filePath)
		if !isApproved {
			continue
		}
		commits, _ := fileProcessor(filePath)

		for _, commit := range commits {
			author := commit.author

			_, ok := subjectsList[author]
			if !ok {
				subjectsList[author] = &SubjectInfo{
					Lines:   0,
					Commits: make(map[string]struct{}),
					Files:   make(map[string]struct{}),
				}
			}
			subjectsList[author].Lines += commit.lineCount
			subjectsList[author].Files[filePath] = struct{}{}
			subjectsList[author].Commits[commit.commit] = struct{}{}
		}
	}

	return subjectsList, nil
}

func SortSubject(subjects SubjectsMap, orderBy SortOrder) []string {
	keys := make([]string, 0, len(subjects))
	for name := range subjects {
		keys = append(keys, name)
	}

	sort.Slice(keys, func(i, j int) bool {
		switch orderBy {
		case SortOrderLines:
			if subjects[keys[i]].Lines != subjects[keys[j]].Lines {
				return subjects[keys[i]].Lines > subjects[keys[j]].Lines
			}
		case SortOrderCommits:
			if len(subjects[keys[i]].Commits) != len(subjects[keys[j]].Commits) {
				return len(subjects[keys[i]].Commits) > len(subjects[keys[j]].Commits)
			}
		case SortOrderFiles:
			if len(subjects[keys[i]].Files) != len(subjects[keys[j]].Files) {
				return len(subjects[keys[i]].Files) > len(subjects[keys[j]].Files)
			}
		}

		if subjects[keys[i]].Lines != subjects[keys[j]].Lines {
			return subjects[keys[i]].Lines > subjects[keys[j]].Lines
		}
		if len(subjects[keys[i]].Commits) != len(subjects[keys[j]].Commits) {
			return len(subjects[keys[i]].Commits) > len(subjects[keys[j]].Commits)
		}
		if len(subjects[keys[i]].Files) != len(subjects[keys[j]].Files) {
			return len(subjects[keys[i]].Files) > len(subjects[keys[j]].Files)
		}
		return keys[i] < keys[j]
	})

	return keys
}

func OutputFormatTabular(subjects SubjectsMap) {
	keys := SortSubject(subjects, orderByFlag)
	w := tabwriter.NewWriter(os.Stdout, 0, 0, 1, ' ', 0)
	fmt.Fprintln(w, "Name\tLines\tCommits\tFiles")

	for _, name := range keys {
		info := subjects[name]
		lines := info.Lines
		commits := len(info.Commits)
		files := len(info.Files)
		fmt.Fprintf(w, "%s\t%d\t%d\t%d\n", name, lines, commits, files)
	}

	w.Flush()
}

func OutputFormatCSV(subjects SubjectsMap) error {
	keys := SortSubject(subjects, orderByFlag)

	w := csv.NewWriter(os.Stdout)
	err := w.Write([]string{"Name", "Lines", "Commits", "Files"})
	if err != nil {
		return err
	}
	for _, name := range keys {
		info := subjects[name]
		lines := info.Lines
		commits := len(info.Commits)
		files := len(info.Files)
		err := w.Write([]string{
			name,
			fmt.Sprintf("%d", lines),
			fmt.Sprintf("%d", commits),
			fmt.Sprintf("%d", files),
		})
		if err != nil {
			return err
		}
	}

	w.Flush()
	return nil
}

func OutputFormatJSON(subjects SubjectsMap) {
	keys := SortSubject(subjects, orderByFlag)

	result := make([]map[string]interface{}, 0, len(subjects))
	for _, name := range keys {
		info := subjects[name]
		lines := info.Lines
		commits := len(info.Commits)
		files := len(info.Files)
		result = append(result, map[string]interface{}{
			"name":    name,
			"lines":   lines,
			"commits": commits,
			"files":   files,
		})
	}

	jsonData, _ := json.Marshal(result)
	fmt.Println(string(jsonData))
}

func OutputFormatJSONLines(subjects SubjectsMap) error {
	keys := SortSubject(subjects, orderByFlag)

	encoder := json.NewEncoder(os.Stdout)
	for _, name := range keys {
		info := subjects[name]
		lines := info.Lines
		commits := len(info.Commits)
		files := len(info.Files)
		err := encoder.Encode(map[string]interface{}{
			"name":    name,
			"lines":   lines,
			"commits": commits,
			"files":   files,
		})
		if err != nil {
			return err
		}
	}
	return nil
}

var (
	repositoryFlag   string
	revisionFlag     string
	orderByFlag      SortOrder
	useCommitterFlag bool
	formatFlag       Format
	extensionsFlag   ListStringFlag
	languagesFlag    ListStringFlag
	excludeFlag      ListStringFlag
	restrictToFlag   ListStringFlag
)

func flagVarsInit() {
	orderByFlag = SortOrderLines
	formatFlag = FormatTabular

	flag.StringVarP(&repositoryFlag, "repository", "p", ".", "git repository path")
	flag.StringVarP(&revisionFlag, "revision", "v", "HEAD", "git revision")
	flag.VarP(&orderByFlag, "order-by", "o", "sort order")
	flag.BoolVarP(&useCommitterFlag, "use-committer", "c", false, "true when using commiter")
	flag.VarP(&formatFlag, "format", "f", "output format")
	flag.VarP(&extensionsFlag, "extensions", "t", "list of extensions")
	flag.VarP(&languagesFlag, "languages", "l", "list of languages")
	flag.VarP(&excludeFlag, "exclude", "x", "files to exclude")
	flag.VarP(&restrictToFlag, "restrict-to", "r", "files only to be counted")

	flag.Parse()
}

func main() {
	flagVarsInit()
	subjectsList, err := getSubjectsInfo()
	if err != nil {
		panic(err.Error())
	}
	switch formatFlag {
	case FormatTabular:
		OutputFormatTabular(subjectsList)
	case FormatCSV:
		err := OutputFormatCSV(subjectsList)
		if err != nil {
			panic(err.Error())
		}
	case FormatJSON:
		OutputFormatJSON(subjectsList)
	case FormatJSONLines:
		err := OutputFormatJSONLines(subjectsList)
		if err != nil {
			panic(err.Error())
		}
	}
}
