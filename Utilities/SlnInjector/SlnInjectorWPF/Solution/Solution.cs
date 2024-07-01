using net.r_eg.MvsSln;
using net.r_eg.MvsSln.Core;
using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SlnInjectorWPF
{
    public class Solution
    {
        private Sln sln;
        private const SlnItems DefaultItems = SlnItems.All & ~SlnItems.LoadDefaultData;

        private IList<Project> projects = new List<Project>();
        private IList<SolutionFolder> solutionFolders = new List<SolutionFolder>();

        public Solution(string path, SlnItems itemsToLoad = DefaultItems) {
            sln = new Sln(path, itemsToLoad);

            foreach(var projItem in Props.ProjectItems)
            {
                projects.Add(new Project(projItem));
            }
        }

        public bool IsProjectInSlnDir(Project project)
        {
            var slnDir = sln.Result.SolutionDir;
            return project.AbsolutePath.StartsWith(slnDir);
        }

        public void AddProject(Project project)
        {
            var newPath = Path.GetRelativePath(Props.SolutionDir, project.AbsolutePath);

            var newItem = new ProjectItem(project.Props)
            {
                path = newPath
            };

            projects.Add(new Project(newItem));

            var newFolders = project.GetParentSolutionFolders();
            foreach (var folder in newFolders)
            {
                if (!solutionFolders.Contains(folder))
                {
                    solutionFolders.Add(folder);
                }
            }
        }

        public async Task Save(string outPath)
        {
            var slnProto = new LhDataHelper();
            var projectItems = Projects.Select(project => project.Props);

            slnProto
                .SetHeader(Props.Header)
                .SetProjects(projectItems)
                .SetProjectConfigs(Props.ProjectConfigs)
                .SetSolutionConfigs(Props.SolutionConfigs)
                .SetDependencies(Props.ProjectDependencies)
                .SetFolders(solutionFolders)
                .SetExt(Props.ExtItems);

            using var writer = new SlnWriter(outPath, slnProto);
            await writer.WriteAsync();

        }

        public ISlnResult Props { get { return sln.Result; } }

        public IEnumerable<Project> Projects { get => projects; }
    }
}
