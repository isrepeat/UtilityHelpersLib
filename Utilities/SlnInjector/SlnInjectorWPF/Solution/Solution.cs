using net.r_eg.MvsSln;
using net.r_eg.MvsSln.Core;
using net.r_eg.MvsSln.Extensions;
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
        [Flags]
        public enum SaveOptions : uint
        {
            None = 1,
            RegenerateDestinationGuids = 2
        }

        private Sln sln;
        private const SlnItems DefaultItems = SlnItems.All & ~SlnItems.LoadDefaultData;

        private IList<Project> projects = new List<Project>();
        private IList<Project> existingProjects = new List<Project>();
        private IList<Project> newProjects = new List<Project>();
        private IList<SolutionFolder> solutionFolders = new List<SolutionFolder>();

        public Solution(string path, SlnItems itemsToLoad = DefaultItems) {
            sln = new Sln(path, itemsToLoad);

            foreach(var projItem in Props.ProjectItems)
            {
                var project = new Project(projItem);
                projects.Add(project);
                existingProjects.Add(project);
                AddParentSolutionFolders(project);
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

            var newProject = new Project(newItem);
            projects.Add(newProject);
            newProjects.Add(newProject);
            AddParentSolutionFolders(project);
        }

        public async Task Save(string outPath, SaveOptions options = SaveOptions.None)
        {
            var slnProto = new LhDataHelper();
            var projectItems = Projects.Select(project => project.Props);

            if (options.HasFlag(SaveOptions.RegenerateDestinationGuids))
            {
                RegenerateExistingGuids();
            }

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

        private void RegenerateExistingGuids()
        {
            foreach (var project in ExistingProjects)
            {
                var projectItem = project.Props;
                
                do {
                    projectItem.pGuid = Guid.NewGuid().SlnFormat();
                } while (Projects.Select(prj => prj.GUID).Contains(projectItem.pGuid));

                project.Props = projectItem;
            }
        }

        public ISlnResult Props { get { return sln.Result; } }

        public IEnumerable<Project> Projects { get => projects; }
        public IEnumerable<Project> ExistingProjects { get => existingProjects; }
        public IEnumerable<Project> NewProjects { get => newProjects; }

        private void AddParentSolutionFolders(Project project)
        {
            var newFolders = project.GetParentSolutionFolders();
            foreach (var folder in newFolders)
            {
                if (!solutionFolders.Contains(folder))
                {
                    solutionFolders.Add(folder);
                }
            }
        }
    }
}
