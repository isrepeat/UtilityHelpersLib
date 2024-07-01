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

        private String GetSolutionFolderPath()
        {
            String path = "";
            SolutionFolder? parentFolder = projectItem.parent.Value;

            while (parentFolder != null)
            {
                var header = parentFolder?.header;
                if (!header.HasValue)
                {
                    break;
                }

                path = header?.name + (path.Count() > 0 ? "/" : "") + path;
                parentFolder = header?.parent.Value;
            }

            return path;
        }

        public ProjectType Type { get { return projectItem.EpType; } }
        public String Name { get { return projectItem.name; } }
        public String SolutionFolder { get; private set; }
        public String RelativePath { get { return projectItem.path; } }
        public String AbsolutePath { get { return projectItem.fullPath; } }
        public String GUID { get { return projectItem.pGuid; } }
    }
}
