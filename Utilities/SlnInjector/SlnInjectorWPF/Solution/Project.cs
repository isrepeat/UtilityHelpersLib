using net.r_eg.MvsSln.Core;
using net.r_eg.MvsSln.Projects;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SlnInjectorWPF
{
    public class Project
    {
        private ProjectItem projectItem;

        public Project(ProjectItem item)
        {
            projectItem = item;
            SolutionFolder = GetSolutionFolderPath();
        }

        private void ForEachParentSolutionFolder(Action<SolutionFolder> action)
        {
            SolutionFolder? parentFolder = projectItem.parent.Value;

            while (parentFolder != null)
            {
                var header = parentFolder?.header;
                if (!header.HasValue)
                {
                    break;
                }

                action((SolutionFolder)parentFolder);
                parentFolder = header?.parent.Value;
            }
        }

        private String GetSolutionFolderPath()
        {
            String path = "";
            SolutionFolder? parentFolder = projectItem.parent.Value;

            ForEachParentSolutionFolder(folder =>
            {
                path = folder.header.name + (path.Count() > 0 ? "/" : "") + path;
            });

            return path;
        }

        public IList<SolutionFolder> GetParentSolutionFolders()
        {
            var folders = new List<SolutionFolder>();

            ForEachParentSolutionFolder(folder =>
            {
                folders.Add(folder);
            });

            return folders;
        }

        public ProjectType Type { get { return projectItem.EpType; } }
        public String Name { get { return projectItem.name; } }
        public String SolutionFolder { get; private set; }
        public String RelativePath { get { return projectItem.path; } }
        public String AbsolutePath { get { return projectItem.fullPath; } }
        public String GUID { get { return projectItem.pGuid; } }
        public ProjectItem Props {
            get => projectItem;
            set => projectItem = value;
        }
    }
}
